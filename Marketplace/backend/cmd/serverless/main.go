package main

import (
	"log"
	"os"
	"strings"

	"github.com/aws/aws-lambda-go/lambda"
	"github.com/awslabs/aws-lambda-go-api-proxy/httpadapter"

	"github.com/waviate-script/marketplace/backend/internal/endpoints"
	"github.com/waviate-script/marketplace/backend/internal/server"
)

func main() {
	endpointName := strings.TrimSpace(os.Getenv("MARKETPLACE_ENDPOINT_NAME"))
	if endpointName == "" {
		log.Fatalf("MARKETPLACE_ENDPOINT_NAME is required; valid endpoint names: %s", strings.Join(endpoints.ServerlessNames(), ", "))
	}

	handler, err := server.NewEndpointHandler(endpointName)
	if err != nil {
		log.Fatal(err)
	}

	lambda.Start(httpadapter.NewV2(handler).ProxyWithContext)
}
