.PHONY: build

build:
	cd cmd/serverless && GOOS=linux GOARCH=amd64 CGO_ENABLED=0 go build -tags lambda.norpc -o ../../bootstrap .
