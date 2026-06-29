package sourcecode

import (
	"bytes"
	"compress/flate"
	"context"
	"encoding/base64"
	"errors"
	"fmt"
	"io"
	"strings"

	"github.com/waviate-script/marketplace/backend/internal/nativecompiler"
)

const (
	MaxSourceChars = 4096 * 128
	contentPrefix  = "wsc1:"
)

var (
	ErrSourceRequired = errors.New("source code is required")
	ErrSourceTooLarge = fmt.Errorf("source code must be less than %d characters", MaxSourceChars)
	ErrNonASCII       = errors.New("source code must contain only ASCII characters")
)

type tokenKind int

const (
	tokenWord tokenKind = iota
	tokenNumber
	tokenString
	tokenPunct
	tokenPreprocessor
)

type token struct {
	value string
	kind  tokenKind
}

var multiCharOperators = []string{
	">>=", "<<=", "->*", "<=>", "...", "::", "++", "--", "->", ".*",
	"&&", "||", "<<", ">>", "<=", ">=", "==", "!=", "+=", "-=", "*=",
	"/=", "%=", "&=", "|=", "^=", "##",
}

var compactAmbiguousPairs = map[string]struct{}{
	"++": {}, "--": {}, "&&": {}, "||": {}, "<<": {}, ">>": {}, "<=": {},
	">=": {}, "==": {}, "!=": {}, "+=": {}, "-=": {}, "*=": {}, "/=": {},
	"%=": {}, "&=": {}, "|=": {}, "^=": {}, "->": {}, "->*": {}, ".*": {},
	"::": {}, "##": {}, "//": {}, "/*": {},
}

var spacedOperators = map[string]struct{}{
	"=": {}, "+": {}, "-": {}, "*": {}, "/": {}, "%": {}, "==": {}, "!=": {},
	"<": {}, ">": {}, "<=": {}, ">=": {}, "&&": {}, "||": {}, "&": {},
	"|": {}, "^": {}, "+=": {}, "-=": {}, "*=": {}, "/=": {}, "%=": {},
	"&=": {}, "|=": {}, "^=": {}, "<<": {}, ">>": {}, "<<=": {}, ">>=": {},
	"?": {},
}

var controlKeywords = map[string]struct{}{
	"if": {}, "for": {}, "while": {}, "switch": {}, "catch": {},
}

func PrepareForStorage(ctx context.Context, source string) (string, error) {
	if err := ValidateSource(source); err != nil {
		return "", err
	}

	if err := RunStaticRoutine(ctx, source); err != nil {
		return "", err
	}

	return Compress(Compact(source))
}

func ValidateSource(source string) error {
	if strings.TrimSpace(source) == "" {
		return ErrSourceRequired
	}
	if len([]rune(source)) >= MaxSourceChars {
		return ErrSourceTooLarge
	}
	for i := 0; i < len(source); i++ {
		if source[i] > 127 {
			return ErrNonASCII
		}
	}

	return nil
}

func RunStaticRoutine(ctx context.Context, source string) error {
	return nativecompiler.Compile(ctx, source)
}

func NativeCompilerStatus() nativecompiler.Status {
	return nativecompiler.CurrentStatus()
}

func Compact(source string) string {
	tokens := tokenize(source)
	var out strings.Builder
	var previous token
	hasPrevious := false

	for _, current := range tokens {
		if current.kind == tokenPreprocessor {
			trimTrailingHorizontalSpace(&out)
			if out.Len() > 0 && !strings.HasSuffix(out.String(), "\n") {
				out.WriteByte('\n')
			}
			out.WriteString(strings.TrimSpace(current.value))
			out.WriteByte('\n')
			hasPrevious = false
			continue
		}

		if hasPrevious && compactNeedsSpace(previous, current) {
			out.WriteByte(' ')
		}
		out.WriteString(current.value)
		previous = current
		hasPrevious = true
	}

	return strings.TrimSpace(out.String())
}

func Compress(source string) (string, error) {
	var compressed bytes.Buffer
	writer, err := flate.NewWriter(&compressed, flate.BestCompression)
	if err != nil {
		return "", err
	}
	if _, err := writer.Write([]byte(source)); err != nil {
		_ = writer.Close()
		return "", err
	}
	if err := writer.Close(); err != nil {
		return "", err
	}

	return contentPrefix + base64.RawStdEncoding.EncodeToString(compressed.Bytes()), nil
}

func RecoverFormatted(content string) (string, error) {
	source, err := Recover(content)
	if err != nil {
		return "", err
	}

	return Format(source), nil
}

func Recover(content string) (string, error) {
	if !strings.HasPrefix(content, contentPrefix) {
		return content, nil
	}

	encoded := strings.TrimPrefix(content, contentPrefix)
	raw, err := base64.RawStdEncoding.DecodeString(encoded)
	if err != nil {
		return "", fmt.Errorf("decode compressed source: %w", err)
	}

	reader := flate.NewReader(bytes.NewReader(raw))
	defer reader.Close()

	source, err := io.ReadAll(io.LimitReader(reader, int64(MaxSourceChars+1)))
	if err != nil {
		return "", fmt.Errorf("decompress source: %w", err)
	}
	if len(source) >= MaxSourceChars {
		return "", ErrSourceTooLarge
	}

	return string(source), nil
}

func Format(source string) string {
	tokens := tokenize(source)
	var out strings.Builder
	indent := 0
	parenDepth := 0
	bracketDepth := 0
	atLineStart := true
	var previous token
	hasPrevious := false

	writeIndent := func() {
		if atLineStart {
			for i := 0; i < indent; i++ {
				out.WriteString("    ")
			}
			atLineStart = false
		}
	}
	newline := func() {
		trimTrailingHorizontalSpace(&out)
		if out.Len() == 0 || strings.HasSuffix(out.String(), "\n") {
			atLineStart = true
			return
		}
		out.WriteByte('\n')
		atLineStart = true
	}
	nextToken := func(index int) token {
		if index+1 >= len(tokens) {
			return token{}
		}
		return tokens[index+1]
	}

	for index, current := range tokens {
		if current.kind == tokenPreprocessor {
			newline()
			out.WriteString(strings.TrimSpace(current.value))
			newline()
			hasPrevious = false
			continue
		}

		switch current.value {
		case "{":
			trimTrailingHorizontalSpace(&out)
			if !atLineStart && !endsWithSpaceOrNewline(out.String()) {
				out.WriteByte(' ')
			}
			writeIndent()
			out.WriteByte('{')
			newline()
			indent++
		case "}":
			newline()
			if indent > 0 {
				indent--
			}
			writeIndent()
			out.WriteByte('}')
			next := nextToken(index)
			if next.value != ";" && next.value != "else" && next.value != "catch" && next.value != "while" {
				newline()
			}
		case ";":
			writeIndent()
			out.WriteByte(';')
			if parenDepth == 0 {
				newline()
			} else {
				out.WriteByte(' ')
			}
		case ",":
			writeIndent()
			out.WriteString(", ")
		case "(":
			if hasPrevious {
				if _, ok := controlKeywords[previous.value]; ok && !endsWithSpaceOrNewline(out.String()) {
					out.WriteByte(' ')
				}
			}
			writeIndent()
			out.WriteByte('(')
			parenDepth++
		case ")":
			trimTrailingHorizontalSpace(&out)
			writeIndent()
			out.WriteByte(')')
			if parenDepth > 0 {
				parenDepth--
			}
		case "[":
			writeIndent()
			out.WriteByte('[')
			bracketDepth++
		case "]":
			trimTrailingHorizontalSpace(&out)
			writeIndent()
			out.WriteByte(']')
			if bracketDepth > 0 {
				bracketDepth--
			}
		case ":":
			writeIndent()
			out.WriteByte(':')
			if parenDepth == 0 && bracketDepth == 0 {
				out.WriteByte(' ')
			}
		default:
			if hasPrevious && formatNeedsSpace(previous, current) && !atLineStart && !endsWithSpaceOrNewline(out.String()) {
				out.WriteByte(' ')
			}
			writeIndent()
			out.WriteString(current.value)
		}

		previous = current
		hasPrevious = true
	}

	formatted := strings.TrimRight(out.String(), " \t\r\n")
	if formatted == "" {
		return ""
	}

	return formatted + "\n"
}

func tokenize(source string) []token {
	tokens := make([]token, 0, len(source)/4)
	lineStart := true

	for i := 0; i < len(source); {
		current := source[i]

		if isWhitespace(current) {
			if current == '\n' || current == '\r' {
				lineStart = true
			}
			i++
			continue
		}

		if lineStart && current == '#' {
			value, next := readPreprocessor(source, i)
			tokens = append(tokens, token{value: value, kind: tokenPreprocessor})
			i = next
			lineStart = true
			continue
		}

		lineStart = false

		if current == '/' && i+1 < len(source) {
			switch source[i+1] {
			case '/':
				i += 2
				for i < len(source) && source[i] != '\n' && source[i] != '\r' {
					i++
				}
				lineStart = true
				continue
			case '*':
				i += 2
				for i+1 < len(source) && !(source[i] == '*' && source[i+1] == '/') {
					i++
				}
				if i+1 < len(source) {
					i += 2
				}
				continue
			}
		}

		if length := rawStringPrefixLength(source[i:]); length > 0 {
			value, next := readRawString(source, i, length)
			tokens = append(tokens, token{value: value, kind: tokenString})
			i = next
			continue
		}

		if current == '"' {
			value, next := readQuoted(source, i, '"')
			tokens = append(tokens, token{value: value, kind: tokenString})
			i = next
			continue
		}

		if current == '\'' {
			value, next := readQuoted(source, i, '\'')
			tokens = append(tokens, token{value: value, kind: tokenString})
			i = next
			continue
		}

		if isIdentifierStart(current) {
			start := i
			i++
			for i < len(source) && isIdentifierPart(source[i]) {
				i++
			}
			tokens = append(tokens, token{value: source[start:i], kind: tokenWord})
			continue
		}

		if isDigit(current) {
			start := i
			i++
			for i < len(source) && isNumberPart(source, i) {
				i++
			}
			tokens = append(tokens, token{value: source[start:i], kind: tokenNumber})
			continue
		}

		if op := readOperator(source[i:]); op != "" {
			tokens = append(tokens, token{value: op, kind: tokenPunct})
			i += len(op)
			continue
		}

		tokens = append(tokens, token{value: source[i : i+1], kind: tokenPunct})
		i++
	}

	return tokens
}

func readPreprocessor(source string, start int) (string, int) {
	i := start
	for i < len(source) {
		if source[i] == '\n' || source[i] == '\r' {
			if hasLineContinuation(source, i) {
				i++
				continue
			}
			break
		}
		i++
	}
	if i < len(source) {
		return source[start:i], i + 1
	}

	return source[start:i], i
}

func hasLineContinuation(source string, newlineIndex int) bool {
	i := newlineIndex - 1
	for i >= 0 && (source[i] == ' ' || source[i] == '\t') {
		i--
	}

	return i >= 0 && source[i] == '\\'
}

func rawStringPrefixLength(source string) int {
	for _, prefix := range []string{`u8R"`, `uR"`, `UR"`, `LR"`, `R"`} {
		if strings.HasPrefix(source, prefix) {
			return len(prefix)
		}
	}

	return 0
}

func readRawString(source string, start int, prefixLength int) (string, int) {
	delimiterStart := start + prefixLength
	openParen := delimiterStart
	for openParen < len(source) && source[openParen] != '(' {
		openParen++
	}
	if openParen >= len(source) {
		return source[start:], len(source)
	}

	delimiter := source[delimiterStart:openParen]
	terminator := ")" + delimiter + `"`
	searchStart := openParen + 1
	if end := strings.Index(source[searchStart:], terminator); end >= 0 {
		next := searchStart + end + len(terminator)
		return source[start:next], next
	}

	return source[start:], len(source)
}

func readQuoted(source string, start int, quote byte) (string, int) {
	i := start + 1
	for i < len(source) {
		if source[i] == '\\' {
			i += 2
			continue
		}
		if source[i] == quote {
			i++
			break
		}
		i++
	}

	return source[start:i], i
}

func readOperator(source string) string {
	for _, operator := range multiCharOperators {
		if strings.HasPrefix(source, operator) {
			return operator
		}
	}

	return ""
}

func compactNeedsSpace(previous token, current token) bool {
	if isWordLike(previous) && isWordLike(current) {
		return true
	}
	if _, ok := compactAmbiguousPairs[previous.value+current.value]; ok {
		return true
	}

	return false
}

func formatNeedsSpace(previous token, current token) bool {
	if compactNeedsSpace(previous, current) {
		return true
	}
	if current.value == "." || current.value == "->" || current.value == "::" {
		return false
	}
	if previous.value == "." || previous.value == "->" || previous.value == "::" {
		return false
	}
	if previous.value == "}" && (current.value == "else" || current.value == "catch" || current.value == "while") {
		return true
	}
	if current.value == "(" && previous.kind == tokenWord {
		_, ok := controlKeywords[previous.value]
		return ok
	}
	if current.value == ";" || current.value == "," || current.value == ")" || current.value == "]" {
		return false
	}
	if previous.value == "(" || previous.value == "[" {
		return false
	}
	if _, ok := spacedOperators[previous.value]; ok {
		return true
	}
	if _, ok := spacedOperators[current.value]; ok {
		return true
	}

	return false
}

func isWordLike(value token) bool {
	return value.kind == tokenWord || value.kind == tokenNumber || value.kind == tokenString
}

func isWhitespace(value byte) bool {
	return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f' || value == '\v'
}

func isIdentifierStart(value byte) bool {
	return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || value == '_'
}

func isIdentifierPart(value byte) bool {
	return isIdentifierStart(value) || isDigit(value)
}

func isDigit(value byte) bool {
	return value >= '0' && value <= '9'
}

func isNumberPart(source string, index int) bool {
	value := source[index]
	if isIdentifierPart(value) || value == '.' {
		return true
	}
	if value == '+' || value == '-' {
		previous := source[index-1]
		return previous == 'e' || previous == 'E' || previous == 'p' || previous == 'P'
	}

	return false
}

func trimTrailingHorizontalSpace(out *strings.Builder) {
	value := out.String()
	trimmed := strings.TrimRight(value, " \t")
	if len(trimmed) == len(value) {
		return
	}

	out.Reset()
	out.WriteString(trimmed)
}

func endsWithSpaceOrNewline(value string) bool {
	return strings.HasSuffix(value, " ") || strings.HasSuffix(value, "\n")
}
