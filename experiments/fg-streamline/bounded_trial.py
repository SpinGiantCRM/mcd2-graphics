"""Opt-in 600-frame FG trial. Never launches, clicks or changes save files."""
import argparse
import json
import os
from pathlib import Path
import time


def read_rows(path, offset=0):
    with path.open(encoding='utf-8') as stream:
        stream.seek(offset)
        lines = stream.read().splitlines(keepends=True)
    # A live writer may have an incomplete final row. Earlier corruption fails.
    return [json.loads(line) for line in lines if line.endswith('\n')]


def summarize(rows):
    outcomes = [r for r in rows if r.get('kind') == 'fg_present_outcome']
    inputs = [r for r in rows if r.get('kind') in ('game_image_tags', 'game_guide_tags')]
    modes = [r for r in rows if r.get('kind') == 'game_fg_mode']
    errors = sum(r['SDKResult'] != 0 or r['status'] != 0 for r in outcomes)
    guide_errors = sum(r['SDKResult'] != 0 or r['tokenResult'] != 0 or
                       r['frameStamp'] != r['SDKIndex'] for r in inputs)
    return {
        'renderedFrames': len(outcomes),
        'actualPresents': sum(r['actualPresents'] for r in outcomes),
        'presentErrors': errors, 'inputErrors': guide_errors,
        'modes': [{k: r[k] for k in ('requestedMode', 'SDKResult', 'matchingInputs')}
                  for r in modes],
        'boundedGatePassed': len(outcomes) == 600 and errors == guide_errors == 0
            and sum(r['actualPresents'] for r in outcomes) > len(outcomes)
            and bool(modes) and modes[-1]['requestedMode'] == 0
            and all(r['SDKResult'] == 0 for r in modes),
        'WindowsQualified': False, 'releaseQualified': False,
    }


def set_policy(path, revision, on):
    temporary = path.with_name(path.name + '.pending')
    temporary.write_text('[Capture]\nEnabled=0\nGuideTags=1\nImageTags=1\n'
                         f'EnableFG={int(on)}\nTrialRevision={revision}\n', encoding='ascii')
    os.replace(temporary, path)


def run(policy, log, revision, timeout):
    offset = log.stat().st_size
    start = time.monotonic()
    try:
        set_policy(policy, revision, False)
        while time.monotonic() - start < timeout:
            rows = read_rows(log, offset)
            if sum(r.get('kind') == 'game_image_tags' and r['SDKResult'] == 0
                   and r['tokenResult'] == 0 for r in rows) >= 20:
                break
            time.sleep(.1)
        else:
            raise TimeoutError('Fresh gameplay inputs not observed; FG remains Off')
        set_policy(policy, revision, True)
        print('Bounded FG requested. Move normally; it turns Off after 600 rendered frames.', flush=True)
        start = time.monotonic()
        while time.monotonic() - start < timeout:
            rows = read_rows(log, offset)
            outcomes = [r for r in rows if r.get('kind') == 'fg_present_outcome']
            modes = [r for r in rows if r.get('kind') == 'game_fg_mode']
            # Missing guides can briefly turn FG Off. That is not the end of the trial.
            if len(outcomes) >= 600 and modes and modes[-1]['requestedMode'] == 0:
                return summarize(rows)
            time.sleep(.1)
        raise TimeoutError('Full 600-frame trial did not complete; FG forced Off')
    finally:
        set_policy(policy, revision, False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--policy', required=True, type=Path)
    parser.add_argument('--log', required=True, type=Path)
    parser.add_argument('--revision', required=True, type=int)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--timeout', type=int, default=45)
    args = parser.parse_args()
    if not 1 <= args.revision <= 0x7fffffff or not 10 <= args.timeout <= 60:
        parser.error('Use a new positive revision and a timeout of 10–60 seconds')
    if not args.policy.is_file() or not args.log.is_file():
        parser.error('A staged, running FG trial is required')
    result = run(args.policy, args.log, args.revision, args.timeout)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result))
    return 0 if result['boundedGatePassed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
