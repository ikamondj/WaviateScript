package main

import (
	"log"
	"net/http"

	"github.com/waviate-script/marketplace/backend/internal/config"
	"github.com/waviate-script/marketplace/backend/internal/server"
)

func main() {
	cfg := config.Load()

	log.Printf("marketplace server listening on %s", cfg.Address)
	if err := http.ListenAndServe(cfg.Address, server.NewHandler()); err != nil {
		log.Fatal(err)
	}
}
