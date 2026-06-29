package server

import (
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net"
	"net/http"

	"gorm.io/driver/postgres"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"

	"github.com/waviate-script/marketplace/backend/internal/auth"
	"github.com/waviate-script/marketplace/backend/internal/config"
	httpcontroller "github.com/waviate-script/marketplace/backend/internal/controller/http"
	"github.com/waviate-script/marketplace/backend/internal/endpoints"
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/persistence/memory"
	gormpg "github.com/waviate-script/marketplace/backend/internal/persistence/postgres"
	"github.com/waviate-script/marketplace/backend/internal/service"
)

type Components struct {
	Store          persistence.Store
	Service        *service.Service
	AuthManager    *auth.Manager
	AuthMiddleware auth.Middleware
}

func NewHandler() http.Handler {
	handler, err := NewHandlerWithConfig(config.Load())
	if err != nil {
		log.Fatal(err)
	}

	return handler
}

func NewHandlerWithConfig(cfg config.Config) (http.Handler, error) {
	components, err := NewComponents(cfg)
	if err != nil {
		return nil, err
	}

	return httpcontroller.NewRouter(
		components.Service,
		components.AuthMiddleware,
		components.AuthManager,
		cfg.LocalAdmin,
	), nil
}

func NewEndpointHandler(endpointName string) (http.Handler, error) {
	return NewEndpointHandlerWithConfig(config.Load(), endpointName)
}

func NewEndpointHandlerWithConfig(cfg config.Config, endpointName string) (http.Handler, error) {
	endpoint, ok := endpoints.Find(endpointName)
	if !ok {
		return nil, fmt.Errorf("unknown marketplace endpoint %q", endpointName)
	}
	if endpoint.LocalAdminOnly {
		return nil, fmt.Errorf("endpoint %q is local-admin only and cannot be deployed as a public serverless function", endpointName)
	}

	components, err := NewComponents(cfg)
	if err != nil {
		return nil, err
	}

	return httpcontroller.NewEndpointRouter(
		components.Service,
		components.AuthMiddleware,
		components.AuthManager,
		endpoint,
	), nil
}

func NewComponents(cfg config.Config) (Components, error) {
	var store persistence.Store

	switch cfg.Mode {
	case "server", "serverless":
		host, port := parseHostPort(cfg.DBAddr)
		dsn := fmt.Sprintf(
			"host=%s user=%s password=%s dbname=%s port=%s sslmode=disable TimeZone=UTC",
			host, cfg.DBUser, cfg.DBPassword, cfg.DBName, port,
		)

		db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{
			Logger: logger.Default.LogMode(logger.Warn),
		})
		if err != nil {
			return Components{}, fmt.Errorf("connect to marketplace database: %w", err)
		}

		store = gormpg.NewGormStore(db, gormpg.Options{DataDir: cfg.DataDir})
		Info("marketplace store connected", map[string]any{
			"mode": cfg.Mode,
			"host": host,
			"port": port,
			"db":   cfg.DBName,
		})

	default:
		store = memory.NewStore()
		Info("marketplace store connected", map[string]any{"mode": "memory"})
	}

	if store == nil {
		return Components{}, errors.New("marketplace store was not initialized")
	}

	authManager := auth.NewManager(store, auth.Config{
		BaseURL:            cfg.AuthBaseURL,
		Secret:             cfg.AuthSecret,
		GoogleClientID:     cfg.GoogleClientID,
		GoogleClientSecret: cfg.GoogleClientSecret,
		SessionTTL:         cfg.SessionTTL,
	})
	marketplaceService := service.NewService(store)
	authMiddleware := auth.NewMiddleware(authManager)

	return Components{
		Store:          store,
		Service:        marketplaceService,
		AuthManager:    authManager,
		AuthMiddleware: authMiddleware,
	}, nil
}

func Info(message string, fields map[string]any) {
	logStructured("info", message, fields)
}

func Error(message string, fields map[string]any) {
	logStructured("error", message, fields)
}

func logStructured(level string, message string, fields map[string]any) {
	payload := map[string]any{
		"level":   level,
		"message": message,
	}
	for key, value := range fields {
		payload[key] = value
	}

	encoded, err := json.Marshal(payload)
	if err != nil {
		log.Printf("%s: %s", level, message)
		return
	}

	log.Print(string(encoded))
}

// parseHostPort splits a "host:port" string, defaulting to localhost / 5432.
func parseHostPort(addr string) (string, string) {
	host, port, err := net.SplitHostPort(addr)
	if err != nil {
		if addr != "" {
			return addr, "5432"
		}
		return "localhost", "5432"
	}
	if host == "" {
		host = "localhost"
	}
	if port == "" {
		port = "5432"
	}
	return host, port
}
