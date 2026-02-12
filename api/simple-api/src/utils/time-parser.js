function parseGermanTime(timeStr) {
  if (!timeStr || typeof timeStr !== 'string') {
    return null;
  }

  const input = timeStr.toLowerCase().trim();

  const numberWords = {
    'null': 0, 'eins': 1, 'ein': 1, 'eine': 1, 'zwei': 2, 'drei': 3, 'vier': 4,
    'fünf': 5, 'fuenf': 5, 'sechs': 6, 'sieben': 7, 'acht': 8, 'neun': 9,
    'zehn': 10, 'elf': 11, 'zwölf': 12, 'zwoelf': 12,
    'dreizehn': 13, 'vierzehn': 14, 'fünfzehn': 15, 'fuenfzehn': 15,
    'sechzehn': 16, 'siebzehn': 17, 'achtzehn': 18, 'neunzehn': 19,
    'zwanzig': 20, 'einundzwanzig': 21, 'zweiundzwanzig': 22, 'dreiundzwanzig': 23
  };

  const toNumber = (str) => {
    const cleaned = str.trim();
    if (numberWords[cleaned] !== undefined) {
      return numberWords[cleaned];
    }
    const num = parseInt(cleaned, 10);
    return isNaN(num) ? null : num;
  };

  const format = (hours, minutes = 0) => {
    const h = Math.floor(hours) % 24;
    const m = Math.floor(minutes) % 60;
    return { hour: h, minute: m };
  };

  const directMatch = input.match(/^(\d{1,2})[:.](\d{2})$/);
  if (directMatch) {
    return format(parseInt(directMatch[1]), parseInt(directMatch[2]));
  }

  const directWithUhr = input.match(/^(\d{1,2})[:.](\d{2})\s*uhr$/);
  if (directWithUhr) {
    return format(parseInt(directWithUhr[1]), parseInt(directWithUhr[2]));
  }

  const parseMinuteWord = (minStr) => {
    const minuteWords = {
      'fünf': 5, 'fuenf': 5, 'zehn': 10, 'fünfzehn': 15, 'fuenfzehn': 15,
      'zwanzig': 20, 'fünfundzwanzig': 25, 'fuenfundzwanzig': 25,
      'dreißig': 30, 'dreissig': 30, 'fünfunddreißig': 35, 'fuenfunddreissig': 35,
      'vierzig': 40, 'fünfundvierzig': 45, 'fuenfundvierzig': 45,
      'fünfzig': 50, 'fuenfzig': 50, 'fünfundfünfzig': 55, 'fuenfundfuenfzig': 55
    };
    if (minuteWords[minStr] !== undefined) {
      return minuteWords[minStr];
    }
    const num = parseInt(minStr, 10);
    return isNaN(num) ? 0 : num;
  };

  const fullTimeMatch = input.match(/^(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf|dreizehn|vierzehn|fünfzehn|fuenfzehn|sechzehn|siebzehn|achtzehn|neunzehn|zwanzig|einundzwanzig|zweiundzwanzig|dreiundzwanzig)\s*uhr\s*(\d{1,2}|fünf|fuenf|zehn|fünfzehn|fuenfzehn|zwanzig|fünfundzwanzig|fuenfundzwanzig|dreißig|dreissig|fünfunddreißig|fuenfunddreissig|vierzig|fünfundvierzig|fuenfundvierzig|fünfzig|fuenfzig|fünfundfünfzig|fuenfundfuenfzig)?$/);
  if (fullTimeMatch) {
    const hours = toNumber(fullTimeMatch[1]);
    let minutes = 0;
    if (fullTimeMatch[2]) {
      minutes = parseMinuteWord(fullTimeMatch[2]);
    }
    if (hours !== null) {
      return format(hours, minutes);
    }
  }

  const hourOnlyMatch = input.match(/^(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf)\s*uhr$/);
  if (hourOnlyMatch) {
    const hours = toNumber(hourOnlyMatch[1]);
    if (hours !== null) {
      return format(hours, 0);
    }
  }

  const halbMatch = input.match(/^halb\s+(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf)$/);
  if (halbMatch) {
    const hours = toNumber(halbMatch[1]);
    if (hours !== null) {
      return format(hours - 1, 30);
    }
  }

  const viertelNachMatch = input.match(/^viertel\s+nach\s+(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf)$/);
  if (viertelNachMatch) {
    const hours = toNumber(viertelNachMatch[1]);
    if (hours !== null) {
      return format(hours, 15);
    }
  }

  const viertelVorMatch = input.match(/^viertel\s+vor\s+(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf)$/);
  if (viertelVorMatch) {
    const hours = toNumber(viertelVorMatch[1]);
    if (hours !== null) {
      return format(hours - 1, 45);
    }
  }

  const dreiviertelMatch = input.match(/^dreiviertel\s+(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf)$/);
  if (dreiviertelMatch) {
    const hours = toNumber(dreiviertelMatch[1]);
    if (hours !== null) {
      return format(hours - 1, 45);
    }
  }

  const nachMatch = input.match(/^(\d{1,2}|fünf|fuenf|zehn|zwanzig|fünfundzwanzig|fuenfundzwanzig)\s+nach\s+(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf)$/);
  if (nachMatch) {
    let minutes = toNumber(nachMatch[1]);
    if (nachMatch[1] === 'fünfundzwanzig' || nachMatch[1] === 'fuenfundzwanzig') minutes = 25;
    const hours = toNumber(nachMatch[2]);
    if (hours !== null && minutes !== null) {
      return format(hours, minutes);
    }
  }

  const vorMatch = input.match(/^(\d{1,2}|fünf|fuenf|zehn|zwanzig|fünfundzwanzig|fuenfundzwanzig)\s+vor\s+(\d{1,2}|eins?|zwei|drei|vier|fünf|fuenf|sechs|sieben|acht|neun|zehn|elf|zwölf|zwoelf)$/);
  if (vorMatch) {
    let minutes = toNumber(vorMatch[1]);
    if (vorMatch[1] === 'fünfundzwanzig' || vorMatch[1] === 'fuenfundzwanzig') minutes = 25;
    const hours = toNumber(vorMatch[2]);
    if (hours !== null && minutes !== null) {
      return format(hours - 1, 60 - minutes);
    }
  }

  const justNumber = input.match(/^(\d{1,2})$/);
  if (justNumber) {
    const hours = parseInt(justNumber[1], 10);
    if (hours >= 0 && hours <= 23) {
      return format(hours, 0);
    }
  }

  const wordHour = toNumber(input);
  if (wordHour !== null && wordHour >= 0 && wordHour <= 12) {
    return format(wordHour, 0);
  }

  const morgenMatch = input.match(/(?:morgen\s+(?:früh\s+)?)?um\s+(.+)/);
  if (morgenMatch) {
    return parseGermanTime(morgenMatch[1]);
  }

  return null;
}

function normalizeEntities(entities) {
  if (!entities || typeof entities !== 'object') {
    return entities;
  }

  const normalized = { ...entities };

  if (normalized.time) {
    const parsedTime = parseGermanTime(normalized.time);
    if (parsedTime) {
      normalized.time = parsedTime;
    }
  }

  return normalized;
}

module.exports = {
  parseGermanTime,
  normalizeEntities
};
