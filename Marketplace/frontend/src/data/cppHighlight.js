const keywordPattern =
  /^(alignas|alignof|and|and_eq|asm|auto|bitand|bitor|bool|break|case|catch|char|char16_t|char32_t|class|compl|concept|const|consteval|constexpr|constinit|const_cast|continue|co_await|co_return|co_yield|decltype|default|delete|do|double|dynamic_cast|else|enum|explicit|export|extern|false|float|for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|not|not_eq|nullptr|operator|or|or_eq|private|protected|public|register|reinterpret_cast|requires|return|short|signed|sizeof|static|static_assert|static_cast|struct|switch|template|this|thread_local|throw|true|try|typedef|typeid|typename|union|unsigned|using|virtual|void|volatile|wchar_t|while|xor|xor_eq)\b/;
const typePattern = /^(std|size_t|uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|string|vector|array|span|optional|variant|unique_ptr|shared_ptr)\b/;

export function highlightCpp(source) {
  let html = '';

  for (let index = 0; index < source.length;) {
    const rest = source.slice(index);

    if (rest.startsWith('//')) {
      const end = findLineEnd(source, index);
      html += wrap('comment', source.slice(index, end));
      index = end;
      continue;
    }

    if (rest.startsWith('/*')) {
      const end = source.indexOf('*/', index + 2);
      const next = end >= 0 ? end + 2 : source.length;
      html += wrap('comment', source.slice(index, next));
      index = next;
      continue;
    }

    if (source[index] === '#') {
      const end = findLineEnd(source, index);
      html += wrap('preprocessor', source.slice(index, end));
      index = end;
      continue;
    }

    if (source[index] === '"' || source[index] === "'") {
      const [value, next] = readQuoted(source, index, source[index]);
      html += wrap('string', value);
      index = next;
      continue;
    }

    const identifier = rest.match(/^[A-Za-z_][A-Za-z0-9_]*/);
    if (identifier) {
      const value = identifier[0];
      if (keywordPattern.test(value)) {
        html += wrap('keyword', value);
      } else if (typePattern.test(value)) {
        html += wrap('type', value);
      } else {
        html += escapeHtml(value);
      }
      index += value.length;
      continue;
    }

    const number = rest.match(/^(0x[0-9a-fA-F]+|\d+(\.\d+)?([eEpP][+-]?\d+)?)([uUlLfF]*)/);
    if (number) {
      html += wrap('number', number[0]);
      index += number[0].length;
      continue;
    }

    html += escapeHtml(source[index]);
    index++;
  }

  return html;
}

function findLineEnd(source, start) {
  const end = source.indexOf('\n', start);
  return end >= 0 ? end : source.length;
}

function readQuoted(source, start, quote) {
  let index = start + 1;
  while (index < source.length) {
    if (source[index] === '\\') {
      index += 2;
      continue;
    }
    if (source[index] === quote) {
      index++;
      break;
    }
    index++;
  }

  return [source.slice(start, index), index];
}

function wrap(kind, value) {
  return `<span class="code-${kind}">${escapeHtml(value)}</span>`;
}

function escapeHtml(value) {
  return value
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}
