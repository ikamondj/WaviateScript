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
  runtime_environment = {
    MARKETPLACE_MODE              = "server"
    MARKETPLACE_ADDR              = ":8080"
    MARKETPLACE_DB_ADDR           = var.db_addr
    MARKETPLACE_DB_USER           = var.db_user
    MARKETPLACE_DB_NAME           = var.db_name
    MARKETPLACE_DATA_DIR          = var.data_dir
    MARKETPLACE_AUTH_BASE_URL     = var.auth_base_url
    MARKETPLACE_SESSION_TTL_HOURS = tostring(var.session_ttl_hours)
    GOOGLE_CLIENT_ID              = var.google_client_id
  }

  runtime_secrets = {
    MARKETPLACE_DB_PASSWORD = var.db_password_secret_arn
    MARKETPLACE_AUTH_SECRET = var.auth_secret_arn
    GOOGLE_CLIENT_SECRET    = var.google_client_secret_arn
  }
}

resource "aws_iam_role" "apprunner_access" {
  name = "${var.name_prefix}-marketplace-apprunner-access"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action = "sts:AssumeRole"
      Effect = "Allow"
      Principal = {
        Service = "build.apprunner.amazonaws.com"
      }
    }]
  })
}

resource "aws_iam_role_policy_attachment" "apprunner_ecr_access" {
  role       = aws_iam_role.apprunner_access.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSAppRunnerServicePolicyForECRAccess"
}

resource "aws_iam_role" "apprunner_instance" {
  name = "${var.name_prefix}-marketplace-apprunner-instance"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action = "sts:AssumeRole"
      Effect = "Allow"
      Principal = {
        Service = "tasks.apprunner.amazonaws.com"
      }
    }]
  })
}

resource "aws_iam_role_policy" "read_runtime_secrets" {
  name = "${var.name_prefix}-marketplace-read-secrets"
  role = aws_iam_role.apprunner_instance.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect = "Allow"
      Action = [
        "secretsmanager:GetSecretValue",
        "ssm:GetParameter",
        "ssm:GetParameters"
      ]
      Resource = compact([
        var.db_password_secret_arn,
        var.auth_secret_arn,
        var.google_client_secret_arn,
      ])
    }]
  })
}

resource "aws_apprunner_auto_scaling_configuration_version" "marketplace" {
  auto_scaling_configuration_name = "${var.name_prefix}-marketplace"
  min_size                        = var.min_instances
  max_size                        = var.max_instances
  max_concurrency                 = var.max_concurrency
}

resource "aws_apprunner_vpc_connector" "marketplace" {
  count              = length(var.subnet_ids) > 0 ? 1 : 0
  vpc_connector_name = "${var.name_prefix}-marketplace"
  subnets            = var.subnet_ids
  security_groups    = var.security_group_ids
}

resource "aws_apprunner_service" "marketplace" {
  service_name = "${var.name_prefix}-marketplace"

  source_configuration {
    auto_deployments_enabled = var.auto_deploy

    authentication_configuration {
      access_role_arn = aws_iam_role.apprunner_access.arn
    }

    image_repository {
      image_identifier      = var.container_image
      image_repository_type = "ECR"

      image_configuration {
        port = "8080"

        runtime_environment_variables = local.runtime_environment
        runtime_environment_secrets   = local.runtime_secrets
      }
    }
  }

  instance_configuration {
    cpu               = var.cpu
    memory            = var.memory
    instance_role_arn = aws_iam_role.apprunner_instance.arn
  }

  auto_scaling_configuration_arn = aws_apprunner_auto_scaling_configuration_version.marketplace.arn

  health_check_configuration {
    protocol            = "HTTP"
    path                = "/healthz"
    interval            = 10
    timeout             = 5
    healthy_threshold   = 1
    unhealthy_threshold = 5
  }

  dynamic "network_configuration" {
    for_each = length(var.subnet_ids) > 0 ? [1] : []
    content {
      egress_configuration {
        egress_type       = "VPC"
        vpc_connector_arn = aws_apprunner_vpc_connector.marketplace[0].arn
      }
    }
  }
}

variable "aws_region" {
  type    = string
  default = "us-east-1"
}

variable "name_prefix" {
  type    = string
  default = "waviatescript"
}

variable "container_image" {
  type        = string
  description = "ECR image URI for the traditional Go server container."
}

variable "auto_deploy" {
  type    = bool
  default = true
}

variable "cpu" {
  type    = string
  default = "0.25 vCPU"
}

variable "memory" {
  type    = string
  default = "0.5 GB"
}

variable "min_instances" {
  type    = number
  default = 1
}

variable "max_instances" {
  type    = number
  default = 3
}

variable "max_concurrency" {
  type    = number
  default = 80
}

variable "db_addr" {
  type = string
}

variable "db_user" {
  type = string
}

variable "db_name" {
  type    = string
  default = "waviatescript_marketplace"
}

variable "db_password_secret_arn" {
  type        = string
  description = "Secrets Manager or SSM parameter ARN containing the database password."
}

variable "data_dir" {
  type    = string
  default = "../persistence/data"
}

variable "auth_base_url" {
  type        = string
  description = "Public API base URL used to construct OAuth callback URLs."
}

variable "auth_secret_arn" {
  type        = string
  description = "Secrets Manager or SSM parameter ARN containing MARKETPLACE_AUTH_SECRET."
}

variable "session_ttl_hours" {
  type    = number
  default = 720
}

variable "google_client_id" {
  type = string
}

variable "google_client_secret_arn" {
  type        = string
  description = "Secrets Manager or SSM parameter ARN containing GOOGLE_CLIENT_SECRET."
}

variable "subnet_ids" {
  type    = list(string)
  default = []
}

variable "security_group_ids" {
  type    = list(string)
  default = []
}

output "service_url" {
  value = aws_apprunner_service.marketplace.service_url
}

output "healthcheck_url" {
  value = "https://${aws_apprunner_service.marketplace.service_url}/healthz"
}
