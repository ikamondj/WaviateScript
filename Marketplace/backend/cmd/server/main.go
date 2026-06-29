package main

import (
	"context"
	"errors"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/waviate-script/marketplace/backend/internal/config"
	"github.com/waviate-script/marketplace/backend/internal/server"
)

func main() {
	cfg := config.Load()
	handler, err := server.NewHandlerWithConfig(cfg)
	if err != nil {
		server.Error("marketplace server setup failed", map[string]any{"error": err.Error()})
		os.Exit(1)
	}

	httpServer := &http.Server{
		Addr:              cfg.Address,
		Handler:           handler,
		ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout:       30 * time.Second,
		WriteTimeout:      60 * time.Second,
		IdleTimeout:       120 * time.Second,
	}

	serverErrors := make(chan error, 1)
	go func() {
		server.Info("marketplace server listening", map[string]any{"address": cfg.Address})
		serverErrors <- httpServer.ListenAndServe()
	}()

	shutdownSignals := make(chan os.Signal, 1)
	signal.Notify(shutdownSignals, os.Interrupt, syscall.SIGTERM)

	select {
	case signalValue := <-shutdownSignals:
		server.Info("marketplace server shutdown requested", map[string]any{"signal": signalValue.String()})
	case err := <-serverErrors:
		if !errors.Is(err, http.ErrServerClosed) {
			server.Error("marketplace server failed", map[string]any{"error": err.Error()})
			os.Exit(1)
		}
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	if err := httpServer.Shutdown(ctx); err != nil {
		server.Error("marketplace server graceful shutdown failed", map[string]any{"error": err.Error()})
		_ = httpServer.Close()
		os.Exit(1)
	}

	server.Info("marketplace server stopped", nil)
}
