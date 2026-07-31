package auth

import "context"

type AuthManager interface {
	Providers() []Provider
	AuthorizationURL(providerName string, client string) (string, error)
	CompleteCallback(ctx context.Context, code string, rawState string) (CallbackResult, error)
}
