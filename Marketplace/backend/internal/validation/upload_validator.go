package validation

import (
	"errors"
	"fmt"
	"strings"

	"github.com/waviate-script/marketplace/backend/internal/domain"
)

var supportedPayloadFormats = map[string]struct{}{
	"unknown":    {},
	"raw":        {},
	"compressed": {},
	"encrypted":  {},
}

func ValidateUpload(request domain.UploadRequest) error {
	if strings.TrimSpace(request.Title) == "" {
		return errors.New("title is required")
	}
	if strings.TrimSpace(request.AuthorID) == "" {
		return errors.New("authorId is required")
	}
	if len(request.Tags) > 12 {
		return errors.New("uploads can include at most 12 tags")
	}

	format := strings.TrimSpace(request.PayloadFormat)
	if format == "" {
		format = "unknown"
	}
	if _, ok := supportedPayloadFormats[format]; !ok {
		return fmt.Errorf("unsupported payload format %q", request.PayloadFormat)
	}

	// TODO: Validate actual Waviate script package once raw/compressed/encrypted handling is chosen.
	if strings.TrimSpace(request.SourceText) == "" {
		return errors.New("sourceText or packaged script payload is required")
	}

	return nil
}
