//go:build !nativecompiler || (!cgo && !windows)

package nativecompiler

import (
	"context"
	"errors"
	"testing"
)

func TestStubCompileSkipsWhenNotRequired(t *testing.T) {
	t.Setenv("MARKETPLACE_REQUIRE_NATIVE_COMPILER", "")

	if err := Compile(context.Background(), "float SampleProcess(WaviateSample& wav) { return 0.0f; }"); err != nil {
		t.Fatalf("expected stub compile to pass when not required, got %v", err)
	}

	status := CurrentStatus()
	if status.Available {
		t.Fatalf("expected stub status to report unavailable")
	}
	if status.Required {
		t.Fatalf("expected stub status not to be required")
	}
}

func TestStubCompileFailsWhenRequired(t *testing.T) {
	t.Setenv("MARKETPLACE_REQUIRE_NATIVE_COMPILER", "1")

	err := Compile(context.Background(), "float SampleProcess(WaviateSample& wav) { return 0.0f; }")
	if !errors.Is(err, ErrUnavailable) {
		t.Fatalf("expected ErrUnavailable, got %v", err)
	}

	status := CurrentStatus()
	if !status.Required {
		t.Fatalf("expected stub status to report required")
	}
}
