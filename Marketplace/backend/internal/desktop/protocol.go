package desktop

import (
	"fmt"
	"net/url"
)

const Scheme = "waviatescript"

type LaunchRequest struct {
	EntryID string `json:"entryId"`
	URI     string `json:"uri"`
}

func InstallURI(entryID string) string {
	// TODO: Confirm the final route once the VST/standalone app registers its URL protocol.
	return fmt.Sprintf("%s://marketplace/install/%s", Scheme, url.PathEscape(entryID))
}
