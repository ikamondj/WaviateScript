package main

import (
	"github.com/aws/aws-lambda-go/lambda"
	"github.com/awslabs/aws-lambda-go-api-proxy/httpadapter"

	"github.com/waviate-script/marketplace/backend/internal/server"
)

func main() {
	handler := server.NewHandler()
	lambda.Start(httpadapter.NewV2(handler).ProxyWithContext)
}
