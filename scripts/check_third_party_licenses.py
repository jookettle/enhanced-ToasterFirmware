#!/usr/bin/env python3
"""
Simple script to extract `lib_deps` entries from `platformio.ini`,
attempt to resolve GitHub owner/repo and download common license files.

Usage:
  python scripts/check_third_party_licenses.py

Notes:
- This script attempts heuristics to map entries to GitHub repos.
- To avoid rate limits, set env var `GITHUB_TOKEN` with a personal token.
"""

import configparser
import os
import re
import sys
from urllib.parse import urlparse

try:
    import requests
except Exception:
    requests = None


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLATFORMIO_INI = os.path.join(ROOT, 'platformio.ini')
OUT_DIR = os.path.join(ROOT, 'third_party_licenses')
os.makedirs(OUT_DIR, exist_ok=True)


def parse_lib_deps():
    parser = configparser.ConfigParser()
    parser.read(PLATFORMIO_INI)
    libs = []
    if 'env' in parser:
        # platformio.ini may use repeated sections; fallback to raw read
        pass

    raw = open(PLATFORMIO_INI, 'r', encoding='utf-8').read()
    m = re.search(r'lib_deps\s*=\s*(?:\n|)([\s\S]*?)(?:\n\S|\Z)', raw)
    if not m:
        # fallback: try to find 'lib_deps =' and read subsequent indented lines
        libs = []
        for line in raw.splitlines():
            if line.strip().startswith('lib_deps'):
                # start collecting
                collect = True
                continue
    else:
        block = m.group(1)
        for line in block.splitlines():
            s = line.strip()
            if not s:
                continue
            # remove inline comments
            s = s.split('#', 1)[0].strip()
            if s:
                libs.append(s)

    return libs


def guess_github_repo(entry):
    # If it's a Git url, parse owner/repo
    if entry.startswith('http'):
        try:
            u = urlparse(entry)
            path = u.path.rstrip('.git').strip('/')
            parts = path.split('/')
            if len(parts) >= 2:
                owner = parts[-2]
                repo = parts[-1]
                return owner, repo
        except Exception:
            return None

    # If it's in owner/repo format
    if '/' in entry and not entry.startswith('adafruit') and not entry.startswith('Makuna'):
        parts = entry.split('/')
        if len(parts) == 2:
            return parts[0], parts[1]

    # heuristics for adafruit entries
    if entry.lower().startswith('adafruit'):
        # try to normalize e.g. 'adafruit/Adafruit GFX Library' -> Adafruit-GFX-Library
        name = entry.split('/', 1)[-1].strip()
        guess = name.replace(' ', '-').replace('_', '-')
        return 'adafruit', guess

    # Makuna/NeoPixelBus
    if entry.lower().startswith('makuna'):
        parts = entry.split('/')
        if len(parts) == 2:
            return parts[0], parts[1]

    return None


def fetch_license(owner, repo, token=None):
    candidates = ['LICENSE', 'LICENSE.md', 'LICENSE.txt', 'license', 'COPYING']
    session = None
    if requests:
        session = requests.Session()
        if token:
            session.headers.update({'Authorization': f'token {token}'})

    for cand in candidates:
        url = f'https://raw.githubusercontent.com/{owner}/{repo}/master/{cand}'
        try:
            if session:
                r = session.get(url, timeout=10)
                if r.status_code == 200 and len(r.text) > 20:
                    return r.text, url
            else:
                # minimal urllib fallback
                from urllib.request import urlopen
                try:
                    with urlopen(url, timeout=10) as fh:
                        text = fh.read().decode('utf-8', errors='ignore')
                        if len(text) > 20:
                            return text, url
                except Exception:
                    pass
        except Exception:
            pass

    return None, None


def main():
    libs = parse_lib_deps()
    if not libs:
        print('lib_deps 항목을 찾지 못했습니다. platformio.ini를 확인하세요.')
        return

    token = os.environ.get('GITHUB_TOKEN')

    summary = []
    for entry in libs:
        print('처리:', entry)
        gh = guess_github_repo(entry)
        if not gh:
            print('  -> GitHub 저장소 추정 불가. 수동 확인 필요.')
            summary.append((entry, None, None))
            continue

        owner, repo = gh
        lic_text, lic_url = fetch_license(owner, repo, token)
        if lic_text:
            fname = f'{owner}_{repo}_LICENSE.txt'.replace('/', '_')
            outpath = os.path.join(OUT_DIR, fname)
            with open(outpath, 'w', encoding='utf-8') as fh:
                fh.write(f'# Source: {owner}/{repo}\n')
                fh.write(f'# URL: {lic_url}\n\n')
                fh.write(lic_text)
            print('  -> 라이선스 수집됨:', outpath)
            summary.append((entry, f'{owner}/{repo}', lic_url))
        else:
            print('  -> 라이선스 파일을 찾지 못함:', owner, repo)
            summary.append((entry, f'{owner}/{repo}', None))

    print('\n요약: 수집 결과는 third_party_licenses 디렉터리를 확인하세요.')


if __name__ == '__main__':
    main()
