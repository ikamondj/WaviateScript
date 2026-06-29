package nativecompiler

import (
	"errors"
	"fmt"
	"os"
	"strings"
)

var ErrUnavailable = errors.New("native Waviate compiler is not linked")

const diagnosticBufferSize = 64 * 1024

type Status struct {
	Available bool   `json:"available"`
	Required  bool   `json:"required"`
	Mode      string `json:"mode"`
	Detail    string `json:"detail,omitempty"`
}

type CompileError struct {
	Diagnostics string
}

func (err CompileError) Error() string {
	if err.Diagnostics == "" {
		return "native Waviate compiler rejected the source"
	}

	return fmt.Sprintf("native Waviate compiler rejected the source: %s", err.Diagnostics)
}

func required() bool {
	switch strings.ToLower(strings.TrimSpace(os.Getenv("MARKETPLACE_REQUIRE_NATIVE_COMPILER"))) {
	case "1", "true", "yes", "on":
		return true
	default:
		return false
	}
}
