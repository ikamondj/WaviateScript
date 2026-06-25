package config

import (
	"os"
	"strings"
)

type Config struct {
	Address    string
	Mode       string
	DBAddr     string
	DBUser     string
	DBName     string
	DBPassword string
	DataDir    string
	LocalAdmin bool
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

	dbAddr := os.Getenv("MARKETPLACE_DB_ADDR")
	if dbAddr == "" {
		dbAddr = "localhost:5432"
	}

	dbUser := os.Getenv("MARKETPLACE_DB_USER")
	if dbUser == "" {
		dbUser = "postgres"
	}

	dbPassword := os.Getenv("MARKETPLACE_DB_PASSWORD")
	if dbPassword == "" {
		dbPassword = "admin"
	}

	dbName := os.Getenv("MARKETPLACE_DB_NAME")
	if dbName == "" {
		dbName = "waviatescript_marketplace"
	}

	dataDir := os.Getenv("MARKETPLACE_DATA_DIR")
	if dataDir == "" {
		dataDir = "../persistence/data"
	}

	localAdmin := truthy(os.Getenv("MARKETPLACE_LOCAL_ADMIN"))

	return Config{
		Address:    address,
		Mode:       mode,
		DBAddr:     dbAddr,
		DBUser:     dbUser,
		DBName:     dbName,
		DBPassword: dbPassword,
		DataDir:    dataDir,
		LocalAdmin: localAdmin,
	}
}

func truthy(value string) bool {
	switch strings.ToLower(strings.TrimSpace(value)) {
	case "1", "true", "yes", "on":
		return true
	default:
		return false
	}
}
