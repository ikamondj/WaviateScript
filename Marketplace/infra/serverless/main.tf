terraform {
  required_version = ">= 1.6.0"

  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = var.aws_region
}

locals {
  endpoints = {
    for endpoint in jsondecode(file("${path.module}/../endpoints.json")) :
    endpoint.name => endpoint
  }

  common_environment = {
    MARKETPLACE_MODE              = "serverless"
    MARKETPLACE_DB_ADDR           = var.db_addr
    MARKETPLACE_DB_USER           = var.db_user
    MARKETPLACE_DB_NAME           = var.db_name
    MARKETPLACE_DATA_DIR          = var.data_dir
    MARKETPLACE_AUTH_BASE_URL     = var.auth_base_url
    MARKETPLACE_SESSION_TTL_HOURS = tostring(var.session_ttl_hours)
    GOOGLE_CLIENT_ID              = var.google_client_id
  }

  sensitive_environment = {
    MARKETPLACE_DB_PASSWORD       = var.db_password
    MARKETPLACE_AUTH_SECRET       = var.auth_secret
    GOOGLE_CLIENT_SECRET          = var.google_client_secret
  }
}

# OAuth bearer-token validation and marketplace role/plan checks are enforced in
# the Go endpoint wrapper. API Gateway intentionally stays a thin transport layer
# so account mapping, standard/premium limits, and creator upload permissions
# remain portable across AWS/GCP/Azure adapters.
resource "aws_apigatewayv2_api" "marketplace" {
  name          = "${var.name_prefix}-marketplace"
  protocol_type = "HTTP"

  cors_configuration {
    allow_headers = ["authorization", "content-type"]
    allow_methods = ["GET", "POST", "OPTIONS"]
    allow_origins = var.allowed_origins
    max_age       = 300
  }
}

resource "aws_cloudwatch_log_group" "lambda" {
  for_each          = local.endpoints
  name              = "/aws/lambda/${var.name_prefix}-${each.value.cloudFunctionName}"
  retention_in_days = var.log_retention_days
}

resource "aws_iam_role" "lambda" {
  name = "${var.name_prefix}-marketplace-lambda"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action = "sts:AssumeRole"
      Effect = "Allow"
      Principal = {
        Service = "lambda.amazonaws.com"
      }
    }]
  })
}

resource "aws_iam_role_policy_attachment" "basic_lambda" {
  role       = aws_iam_role.lambda.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
}

resource "aws_iam_role_policy_attachment" "vpc_lambda" {
  count      = length(var.subnet_ids) > 0 ? 1 : 0
  role       = aws_iam_role.lambda.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaVPCAccessExecutionRole"
}

resource "aws_lambda_function" "endpoint" {
  for_each = local.endpoints

  function_name    = "${var.name_prefix}-${each.value.cloudFunctionName}"
  role             = aws_iam_role.lambda.arn
  runtime          = "provided.al2023"
  handler          = "bootstrap"
  filename         = var.lambda_zip_path
  source_code_hash = filebase64sha256(var.lambda_zip_path)
  memory_size      = var.lambda_memory_mb
  timeout          = var.lambda_timeout_seconds

  environment {
    variables = merge(
      local.common_environment,
      local.sensitive_environment,
      {
        MARKETPLACE_ENDPOINT_NAME = each.key
      },
    )
  }

  dynamic "vpc_config" {
    for_each = length(var.subnet_ids) > 0 ? [1] : []
    content {
      subnet_ids         = var.subnet_ids
      security_group_ids = var.security_group_ids
    }
  }

  depends_on = [
    aws_cloudwatch_log_group.lambda,
    aws_iam_role_policy_attachment.basic_lambda,
    aws_iam_role_policy_attachment.vpc_lambda,
  ]
}

resource "aws_apigatewayv2_integration" "endpoint" {
  for_each = local.endpoints

  api_id                 = aws_apigatewayv2_api.marketplace.id
  integration_type       = "AWS_PROXY"
  integration_method     = "POST"
  integration_uri        = aws_lambda_function.endpoint[each.key].invoke_arn
  payload_format_version = "2.0"
}

resource "aws_apigatewayv2_route" "endpoint" {
  for_each = local.endpoints

  api_id    = aws_apigatewayv2_api.marketplace.id
  route_key = "${each.value.method} ${each.value.path}"
  target    = "integrations/${aws_apigatewayv2_integration.endpoint[each.key].id}"
}

resource "aws_lambda_permission" "api_gateway" {
  for_each = local.endpoints

  statement_id  = "AllowExecutionFromApiGateway-${each.value.cloudFunctionName}"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.endpoint[each.key].function_name
  principal     = "apigateway.amazonaws.com"
  source_arn    = "${aws_apigatewayv2_api.marketplace.execution_arn}/*/*"
}

resource "aws_apigatewayv2_stage" "default" {
  api_id      = aws_apigatewayv2_api.marketplace.id
  name        = "$default"
  auto_deploy = true
}

variable "aws_region" {
  type    = string
  default = "us-east-1"
}

variable "name_prefix" {
  type    = string
  default = "waviatescript"
}

variable "lambda_zip_path" {
  type        = string
  description = "Path to a zip containing the cmd/serverless bootstrap binary."
}

variable "lambda_memory_mb" {
  type    = number
  default = 512
}

variable "lambda_timeout_seconds" {
  type    = number
  default = 30
}

variable "log_retention_days" {
  type    = number
  default = 14
}

variable "allowed_origins" {
  type    = list(string)
  default = ["*"]
}

variable "db_addr" {
  type = string
}

variable "db_user" {
  type = string
}

variable "db_password" {
  type      = string
  sensitive = true
}

variable "db_name" {
  type    = string
  default = "waviatescript_marketplace"
}

variable "data_dir" {
  type    = string
  default = "../persistence/data"
}

variable "auth_base_url" {
  type        = string
  description = "Public API base URL used to construct OAuth callback URLs."
}

variable "auth_secret" {
  type      = string
  sensitive = true
}

variable "session_ttl_hours" {
  type    = number
  default = 720
}

variable "google_client_id" {
  type = string
}

variable "google_client_secret" {
  type      = string
  sensitive = true
}

variable "subnet_ids" {
  type    = list(string)
  default = []
}

variable "security_group_ids" {
  type    = list(string)
  default = []
}

output "api_base_url" {
  value = aws_apigatewayv2_api.marketplace.api_endpoint
}

output "endpoint_function_names" {
  value = {
    for name, function in aws_lambda_function.endpoint :
    name => function.function_name
  }
}
