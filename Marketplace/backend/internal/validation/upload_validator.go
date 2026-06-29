package validation

import (
	"errors"
	"fmt"
	"regexp"
	"strings"

	"github.com/waviate-script/marketplace/backend/internal/domain"
)

var tagPattern = regexp.MustCompile(`^[a-z0-9][a-z0-9-]{0,63}$`)

func ValidateUpload(request domain.UploadRequest) error {
	if strings.TrimSpace(request.Name) == "" {
		return errors.New("name is required")
	}
	if len([]rune(request.Name)) > 120 {
		return errors.New("name must be 120 characters or fewer")
	}
	if strings.TrimSpace(request.AuthorID) == "" {
		return errors.New("authorId is required")
	}
	if strings.TrimSpace(request.Description) == "" {
		return errors.New("description is required")
	}
	if len([]rune(request.Description)) > 2000 {
		return errors.New("description must be 2000 characters or fewer")
	}
	if len(request.Tags) > 12 {
		return errors.New("uploads can include at most 12 tags")
	}
	for _, tag := range request.Tags {
		normalized := strings.TrimSpace(strings.ToLower(tag))
		if !tagPattern.MatchString(normalized) {
			return fmt.Errorf("invalid tag %q; use lowercase letters, numbers, and hyphens", tag)
		}
	}

	if strings.TrimSpace(request.Content) == "" {
		return errors.New("content is required")
	}

	return nil
}
