package sourcecode

import (
	"context"
	"errors"
	"strings"
	"testing"
)

func TestValidateSource(t *testing.T) {
	if err := ValidateSource("int main() { return 0; }"); err != nil {
		t.Fatalf("expected valid source: %v", err)
	}

	if err := ValidateSource("int value = 1; // snowman: ☃"); !errors.Is(err, ErrNonASCII) {
		t.Fatalf("expected non-ASCII error, got %v", err)
	}

	tooLarge := strings.Repeat("a", MaxSourceChars)
	if err := ValidateSource(tooLarge); !errors.Is(err, ErrSourceTooLarge) {
		t.Fatalf("expected too-large error, got %v", err)
	}
}

func TestCompactRemovesUnnecessaryWhitespaceAndComments(t *testing.T) {
	source := `
int   main  (  ) {
    int value = 1 + 2; // comment
    return value;
}
`

	got := Compact(source)
	want := `int main(){int value=1+2;return value;}`
	if got != want {
		t.Fatalf("unexpected compact source:\nwant %q\n got %q", want, got)
	}
}

func TestCompactPreservesTokenDelimiters(t *testing.T) {
	source := `int value = a + +b; int other = c - -d;`
	got := Compact(source)
	if !strings.Contains(got, "a+ +b") {
		t.Fatalf("expected compact source to preserve + + delimiter, got %q", got)
	}
	if !strings.Contains(got, "c- -d") {
		t.Fatalf("expected compact source to preserve - - delimiter, got %q", got)
	}
}

func TestPrepareForStorageAndRecoverFormatted(t *testing.T) {
	t.Setenv("MARKETPLACE_REQUIRE_NATIVE_COMPILER", "")

	source := `
float   SampleProcess(const WaviateSample& wav) {
    return wav.getIncomingSample();
}
`

	stored, err := PrepareForStorage(context.Background(), source)
	if err != nil {
		t.Fatalf("prepare source: %v", err)
	}
	if !strings.HasPrefix(stored, contentPrefix) {
		t.Fatalf("expected compressed content prefix, got %q", stored)
	}

	recovered, err := Recover(stored)
	if err != nil {
		t.Fatalf("recover source: %v", err)
	}
	if recovered != "float SampleProcess(const WaviateSample&wav){return wav.getIncomingSample();}" {
		t.Fatalf("unexpected recovered compact source %q", recovered)
	}

	formatted, err := RecoverFormatted(stored)
	if err != nil {
		t.Fatalf("recover formatted source: %v", err)
	}
	for _, expected := range []string{"float SampleProcess", "{\n", "return wav.getIncomingSample();", "}\n"} {
		if !strings.Contains(formatted, expected) {
			t.Fatalf("expected formatted source to contain %q, got:\n%s", expected, formatted)
		}
	}
}
