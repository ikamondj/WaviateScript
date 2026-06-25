//go:build !nativecompiler || (!cgo && !windows)

package nativecompiler

import (
	"context"
)

func Compile(ctx context.Context, _ string) error {
	select {
	case <-ctx.Done():
		return ctx.Err()
	default:
	}

	if required() {
		return ErrUnavailable
	}

	return nil
}

func CurrentStatus() Status {
	detail := "build without -tags nativecompiler and cgo; source compile validation is skipped"
	if required() {
		detail = "MARKETPLACE_REQUIRE_NATIVE_COMPILER is set but native compiler is not linked"
	}

	return Status{
		Available: false,
		Required:  required(),
		Mode:      "stub",
		Detail:    detail,
	}
}
