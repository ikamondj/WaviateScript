package config

import "os"

type Config struct {
	Address string
	Mode    string
}

func Load() Config {
	address := os.Getenv("MARKETPLACE_ADDR")
	if address == "" {
		address = ":8080"
	}

	mode := os.Getenv("MARKETPLACE_MODE")
	if mode == "" {
		mode = "server"
	}

	return Config{
		Address: address,
		Mode:    mode,
	}
}
