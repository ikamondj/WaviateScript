//go:build nativecompiler && cgo && !windows

package nativecompiler

/*
#cgo CFLAGS: -I${SRCDIR}/../../native/include
#cgo linux LDFLAGS: -L${SRCDIR}/../../native/build/lib -lwaviate_marketplace_compiler -lstdc++ -ldl -lm -pthread
#cgo darwin LDFLAGS: -L${SRCDIR}/../../native/build/lib -lwaviate_marketplace_compiler -lc++ -framework CoreFoundation
#include "waviate_marketplace_compiler.h"
#include <stdlib.h>
*/
import "C"

import (
	"context"
	"strings"
	"unsafe"
)

func Compile(ctx context.Context, source string) error {
	select {
	case <-ctx.Done():
		return ctx.Err()
	default:
	}

	cSource := C.CString(source)
	defer C.free(unsafe.Pointer(cSource))

	diagnostics := make([]C.char, diagnosticBufferSize)
	result := C.waviate_marketplace_compile_wlsl(
		cSource,
		&diagnostics[0],
		C.size_t(len(diagnostics)),
	)
	if result == 0 {
		return nil
	}

	return CompileError{
		Diagnostics: strings.TrimSpace(C.GoString(&diagnostics[0])),
	}
}

func CurrentStatus() Status {
	return Status{
		Available: true,
		Required:  required(),
		Mode:      "native",
		Detail:    strings.TrimSpace(C.GoString(C.waviate_marketplace_compiler_version())),
	}
}
