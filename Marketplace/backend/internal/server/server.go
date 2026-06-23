package server

import (
	"net/http"

	"github.com/waviate-script/marketplace/backend/internal/auth"
	httpcontroller "github.com/waviate-script/marketplace/backend/internal/controller/http"
	"github.com/waviate-script/marketplace/backend/internal/persistence/memory"
	"github.com/waviate-script/marketplace/backend/internal/service"
)

func NewHandler() http.Handler {
	store := memory.NewStore()
	marketplaceService := service.NewService(store)

	return httpcontroller.NewRouter(marketplaceService, auth.Middleware{})
}
