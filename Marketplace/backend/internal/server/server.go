package server

import (
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
	"github.com/waviate-script/marketplace/backend/internal/persistence"
	"github.com/waviate-script/marketplace/backend/internal/persistence/memory"
	gormpg "github.com/waviate-script/marketplace/backend/internal/persistence/postgres"
	"github.com/waviate-script/marketplace/backend/internal/service"
)

// NewHandler builds the root HTTP handler. The backing store is selected
// based on MARKETPLACE_MODE:
//
//   - "server"     → GORM/Postgres (for npm run local or hosted server)
//   - "serverless" → GORM/Postgres (for AWS Lambda, DB coords come from env)
//   - anything else → in-memory dummy data (quick dev / unit tests)
func NewHandler() http.Handler {
	cfg := config.Load()

	var store persistence.Store

	switch cfg.Mode {
	case "server", "serverless":
		// Both modes talk to Postgres; they only differ in how the
		// binary is started (ListenAndServe vs lambda.Start).
		host, port := parseHostPort(cfg.DBAddr)
		dsn := fmt.Sprintf(
			"host=%s user=%s password=%s dbname=%s port=%s sslmode=disable TimeZone=UTC",
			host, cfg.DBUser, cfg.DBPassword, cfg.DBName, port,
		)

		db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{
			Logger: logger.Default.LogMode(logger.Warn),
		})
		if err != nil {
			log.Fatalf("failed to connect to database: %v", err)
		}

		store = gormpg.NewGormStore(db, gormpg.Options{DataDir: cfg.DataDir})
		log.Printf("Wired to Postgres via GORM (mode=%s, host=%s, port=%s, db=%s)",
			cfg.Mode, host, port, cfg.DBName)

	default:
		store = memory.NewStore()
		log.Println("Wired to in-memory dummy data store")
	}

	marketplaceService := service.NewService(store)
	return httpcontroller.NewRouter(marketplaceService, auth.Middleware{}, cfg.LocalAdmin)
}

// parseHostPort splits a "host:port" string, defaulting to localhost / 5432.
func parseHostPort(addr string) (string, string) {
	host, port, err := net.SplitHostPort(addr)
	if err != nil {
		// addr might be just a hostname without a port
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
