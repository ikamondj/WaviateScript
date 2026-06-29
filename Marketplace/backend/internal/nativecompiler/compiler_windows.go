//go:build nativecompiler && windows

package nativecompiler

import (
	"bytes"
	"context"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
	"syscall"
	"unsafe"
)

const windowsCompilerDLLName = "waviate_marketplace_compiler_runtime.dll"

type windowsCompiler struct {
	dll         *syscall.DLL
	compileProc *syscall.Proc
	versionProc *syscall.Proc
	path        string
}

var windowsState struct {
	once     sync.Once
	compiler *windowsCompiler
	err      error
}

func Compile(ctx context.Context, source string) error {
	select {
	case <-ctx.Done():
		return ctx.Err()
	default:
	}

	compiler, err := loadWindowsCompiler()
	if err != nil {
		if required() {
			return fmt.Errorf("%w: %v", ErrUnavailable, err)
		}

		return nil
	}

	return compiler.compile(source)
}

func CurrentStatus() Status {
	compiler, err := loadWindowsCompiler()
	if err != nil {
		return Status{
			Available: false,
			Required:  required(),
			Mode:      "windows-dll",
			Detail:    err.Error(),
		}
	}

	return Status{
		Available: true,
		Required:  required(),
		Mode:      "windows-dll",
		Detail:    strings.TrimSpace(compiler.version()) + " at " + compiler.path,
	}
}

func loadWindowsCompiler() (*windowsCompiler, error) {
	windowsState.once.Do(func() {
		addWindowsCompilerSearchPaths()

		var loadErrors []string
		for _, candidate := range windowsCompilerCandidates() {
			dll, err := syscall.LoadDLL(candidate)
			if err != nil {
				loadErrors = append(loadErrors, fmt.Sprintf("%s: %v", candidate, err))
				continue
			}

			compileProc, err := dll.FindProc("waviate_marketplace_compile_wlsl")
			if err != nil {
				loadErrors = append(loadErrors, fmt.Sprintf("%s: %v", candidate, err))
				_ = dll.Release()
				continue
			}

			versionProc, err := dll.FindProc("waviate_marketplace_compiler_version")
			if err != nil {
				loadErrors = append(loadErrors, fmt.Sprintf("%s: %v", candidate, err))
				_ = dll.Release()
				continue
			}

			windowsState.compiler = &windowsCompiler{
				dll:         dll,
				compileProc: compileProc,
				versionProc: versionProc,
				path:        candidate,
			}
			return
		}

		windowsState.err = fmt.Errorf("native compiler runtime DLL unavailable; tried %s", strings.Join(loadErrors, "; "))
	})

	if windowsState.compiler == nil {
		return nil, windowsState.err
	}

	return windowsState.compiler, nil
}

func windowsCompilerCandidates() []string {
	if override := strings.TrimSpace(os.Getenv("MARKETPLACE_NATIVE_COMPILER_DLL")); override != "" {
		return []string{override}
	}

	_, file, _, ok := runtime.Caller(0)
	if !ok {
		return []string{windowsCompilerDLLName}
	}

	nativeDir := filepath.Clean(filepath.Join(filepath.Dir(file), "..", "..", "native"))
	return []string{
		filepath.Join(nativeDir, "build", "bin", "Release", windowsCompilerDLLName),
		filepath.Join(nativeDir, "build", "bin", "Debug", windowsCompilerDLLName),
		filepath.Join(nativeDir, "bin", "Release", windowsCompilerDLLName),
		filepath.Join(nativeDir, "bin", "Debug", windowsCompilerDLLName),
		windowsCompilerDLLName,
	}
}

func addWindowsCompilerSearchPaths() {
	candidates := []string{
		strings.TrimSpace(os.Getenv("MARKETPLACE_LLVM_BIN")),
		filepath.Join(os.Getenv("ProgramFiles"), "LLVM", "bin"),
		filepath.Join(os.Getenv("ProgramW6432"), "LLVM", "bin"),
	}

	pathValue := os.Getenv("PATH")
	for _, candidate := range candidates {
		if candidate == "" {
			continue
		}
		if _, err := os.Stat(candidate); err != nil {
			continue
		}
		if strings.Contains(strings.ToLower(pathValue), strings.ToLower(candidate)) {
			continue
		}

		pathValue = candidate + string(os.PathListSeparator) + pathValue
	}

	_ = os.Setenv("PATH", pathValue)
}

func (compiler *windowsCompiler) compile(source string) error {
	sourceBytes := append([]byte(source), 0)
	diagnostics := make([]byte, diagnosticBufferSize)

	result, _, callErr := compiler.compileProc.Call(
		uintptr(unsafe.Pointer(&sourceBytes[0])),
		uintptr(unsafe.Pointer(&diagnostics[0])),
		uintptr(len(diagnostics)),
	)
	if result == 0 {
		return nil
	}

	message := strings.TrimSpace(readNullTerminatedBytes(diagnostics))
	if message == "" && callErr != syscall.Errno(0) {
		message = callErr.Error()
	}

	return CompileError{Diagnostics: message}
}

func (compiler *windowsCompiler) version() string {
	address, _, _ := compiler.versionProc.Call()
	return readNullTerminatedAddress(address)
}

func readNullTerminatedBytes(buffer []byte) string {
	if end := bytes.IndexByte(buffer, 0); end >= 0 {
		buffer = buffer[:end]
	}

	return string(buffer)
}

func readNullTerminatedAddress(address uintptr) string {
	if address == 0 {
		return ""
	}

	buffer := make([]byte, 0, 64)
	for offset := uintptr(0); ; offset++ {
		value := *(*byte)(unsafe.Pointer(address + offset))
		if value == 0 {
			break
		}
		buffer = append(buffer, value)
	}

	return string(buffer)
}
