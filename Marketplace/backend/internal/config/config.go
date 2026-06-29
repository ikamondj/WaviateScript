package config

import (
	"os"
	"strconv"
	"strings"
	"time"
)

type Config struct {
	Address            string
	Mode               string
	DBAddr             string
	DBUser             string
	DBName             string
	DBPassword         string
	DataDir            string
	LocalAdmin         bool
	AuthBaseURL        string
	AuthSecret         string
	GoogleClientID     string
	GoogleClientSecret string
	SessionTTL         time.Duration
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
	authBaseURL := os.Getenv("MARKETPLACE_AUTH_BASE_URL")
	if authBaseURL == "" {
		authBaseURL = "http://localhost:8080"
	}

	sessionTTL := readHours(os.Getenv("MARKETPLACE_SESSION_TTL_HOURS"), 24*30)

	return Config{
		Address:            address,
		Mode:               mode,
		DBAddr:             dbAddr,
		DBUser:             dbUser,
		DBName:             dbName,
		DBPassword:         dbPassword,
		DataDir:            dataDir,
		LocalAdmin:         localAdmin,
		AuthBaseURL:        authBaseURL,
		AuthSecret:         os.Getenv("MARKETPLACE_AUTH_SECRET"),
		GoogleClientID:     os.Getenv("GOOGLE_CLIENT_ID"),
		GoogleClientSecret: os.Getenv("GOOGLE_CLIENT_SECRET"),
		SessionTTL:         time.Duration(sessionTTL) * time.Hour,
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

func readHours(value string, fallback int) int {
	parsed, err := strconv.Atoi(strings.TrimSpace(value))
	if err != nil || parsed <= 0 {
		return fallback
	}

	return parsed
}
