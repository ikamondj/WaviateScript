package main

import (
	"log"
	"net/http"

	"github.com/waviate-script/marketplace/backend/internal/server"
)

func Handler() http.Handler {
	return server.NewHandler()
}

func main() {
	// TODO: Adapt Handler to the selected cloud provider once hosting is chosen.
	_ = Handler()
	log.Println("marketplace serverless handler scaffold initialized")
}
