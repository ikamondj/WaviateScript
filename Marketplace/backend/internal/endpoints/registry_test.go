package endpoints

import (
	"encoding/json"
	"os"
	"path/filepath"
	"testing"
)

func TestRegistryHasUniqueRoutesAndNames(t *testing.T) {
	seenNames := map[string]struct{}{}
	seenRoutes := map[string]struct{}{}

	for _, endpoint := range Registry() {
		if endpoint.Name == "" {
			t.Fatalf("endpoint name is required")
		}
		if endpoint.Method == "" {
			t.Fatalf("endpoint %s method is required", endpoint.Name)
		}
		if endpoint.Path == "" {
			t.Fatalf("endpoint %s path is required", endpoint.Name)
		}

		if _, ok := seenNames[endpoint.Name]; ok {
			t.Fatalf("duplicate endpoint name %q", endpoint.Name)
		}
		seenNames[endpoint.Name] = struct{}{}

		routeKey := endpoint.Method + " " + endpoint.Path
		if _, ok := seenRoutes[routeKey]; ok {
			t.Fatalf("duplicate endpoint route %q", routeKey)
		}
		seenRoutes[routeKey] = struct{}{}
	}
}

func TestProtectedEndpointMetadata(t *testing.T) {
	upload, ok := Find(Upload)
	if !ok {
		t.Fatalf("upload endpoint is missing")
	}
	if !upload.Auth.Required {
		t.Fatalf("upload endpoint must require auth")
	}

	me, ok := Find(AuthMe)
	if !ok {
		t.Fatalf("auth-me endpoint is missing")
	}
	if !me.Auth.Required {
		t.Fatalf("auth-me endpoint must require auth")
	}

	health, ok := Find(Health)
	if !ok {
		t.Fatalf("health endpoint is missing")
	}
	if health.Auth.Required {
		t.Fatalf("health endpoint should remain public")
	}
}

func TestServerlessManifestMatchesRegistry(t *testing.T) {
	manifestPath := filepath.Clean(filepath.Join("..", "..", "..", "infra", "endpoints.json"))
	contents, err := os.ReadFile(manifestPath)
	if err != nil {
		t.Fatalf("read endpoint manifest: %v", err)
	}

	var manifest []Endpoint
	if err := json.Unmarshal(contents, &manifest); err != nil {
		t.Fatalf("parse endpoint manifest: %v", err)
	}

	registryByName := map[string]Endpoint{}
	for _, endpoint := range ServerlessEndpoints() {
		registryByName[endpoint.Name] = endpoint
	}

	if len(manifest) != len(registryByName) {
		t.Fatalf("manifest endpoint count %d != registry endpoint count %d", len(manifest), len(registryByName))
	}

	for _, deployed := range manifest {
		registered, ok := registryByName[deployed.Name]
		if !ok {
			t.Fatalf("manifest includes unknown endpoint %q", deployed.Name)
		}
		if deployed.Method != registered.Method || deployed.Path != registered.Path {
			t.Fatalf("manifest route drift for %s: %s %s != %s %s",
				deployed.Name,
				deployed.Method,
				deployed.Path,
				registered.Method,
				registered.Path)
		}
		if deployed.Auth.Required != registered.Auth.Required {
			t.Fatalf("manifest auth drift for %s", deployed.Name)
		}
		if deployed.CloudFunctionName != registered.CloudFunctionName {
			t.Fatalf("manifest function-name drift for %s", deployed.Name)
		}
	}
}
