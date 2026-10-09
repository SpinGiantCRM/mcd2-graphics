"""Analyze bounded CPU present intervals; never report physical display FPS."""
import argparse
import json
import statistics
from pathlib import Path


def analyze(rows, enabled):
    if len(rows) < 33 or len(rows) > 1203:
        raise ValueError('Expected 30-1200 bounded samples plus three metadata rows')
    scope, before, after = rows[:3]
    if (scope.get('kind') != 'scope' or scope.get('applicationPresentIntervals') is not True
            or scope.get('physicalDisplayFrames') is not False
            or scope.get('gpuQueries') is not False or scope.get('pixelReadbacks') is not False):
        raise ValueError('Unexpected measurement scope')
    frequency = scope.get('qpcFrequency')
    if type(frequency) is not int or frequency <= 0:
        raise ValueError('Invalid CPU counter frequency')
    if before.get('kind') != 'before' or after.get('kind') != 'after':
        raise ValueError('Missing provider boundary states')
    ticks = []
    for row in rows[3:]:
        if row.get('kind') != 'sample' or type(row.get('counter')) is not int:
            raise ValueError('Invalid CPU sample')
        ticks.append(row['counter'])
    intervals = [b - a for a, b in zip(ticks, ticks[1:])]
    if min(intervals) <= 0:
        raise ValueError('CPU counter did not advance')
    ms = [value * 1000 / frequency for value in intervals]
    seconds = sum(intervals) / frequency
    provider = all(row.get('module') is True and row.get('result') == 0 for row in (before, after))
    if any(row.get('active') != int(enabled) for row in (before, after)):
        raise ValueError('Requested FG mode was not active at both boundaries')
    real_rate = generated_rate = total_rate = None
    if enabled:
        if not provider or any(row.get('ready') != 1 or any(row.get(key) != 0
                for key in ('fault', 'errors', 'warnings')) for row in (before, after)):
            raise ValueError('FG provider was unavailable or reported a fault')
        deltas = {}
        for key in ('providerFrame', 'prepared', 'images', 'realPresents', 'generatedPresents'):
            if any(type(row.get(key)) is not int for row in (before, after)):
                raise ValueError('Invalid provider counter')
            deltas[key] = after[key] - before[key]
            if deltas[key] <= 0:
                raise ValueError('FG provider did not advance')
        # More than one observed swapchain or asynchronous accounting can make
        # these intervals unsuitable for matching rendered and generated rates.
        if abs(deltas['realPresents'] - len(intervals)) > 1:
            raise ValueError('Application and SDK real-present counts disagree')
        if deltas['generatedPresents'] > deltas['realPresents'] + 1:
            raise ValueError('Unexpected multiplier for this single-frame sampler')
        real_rate = deltas['realPresents'] / seconds
        generated_rate = deltas['generatedPresents'] / seconds
        total_rate = real_rate + generated_rate
    elif provider:
        if any(row.get(key) != 0 for row in (before, after) for key in ('fault', 'errors', 'warnings')):
            raise ValueError('Disabled provider reported a fault')
        if before.get('ready') != after.get('ready') or before.get('ready') not in (0, 1):
            raise ValueError('Disabled presenter ownership changed during capture')
        for key in ('realPresents', 'generatedPresents'):
            if any(type(row.get(key)) is not int or row[key] < 0 for row in (before, after)):
                raise ValueError('Invalid disabled-provider counter')
            if after[key] < before[key]:
                raise ValueError('Disabled-provider counter moved backwards')
        if after['generatedPresents'] != before['generatedPresents']:
            raise ValueError('Provider generated frames while Off')
        # A loaded bridge with no owned swapchain has no SDK present counter.
        if before.get('ready') == after.get('ready') == 1:
            if abs((after['realPresents'] - before['realPresents']) - len(intervals)) > 1:
                raise ValueError('Application and disabled SDK real-present counts disagree')
            real_rate = (after['realPresents'] - before['realPresents']) / seconds
        generated_rate = 0
    ordered = sorted(ms)
    return {
        'FGOn': enabled, 'intervals': len(ms), 'seconds': seconds,
        'applicationPresentRate': len(ms) / seconds,
        'medianMs': statistics.median(ms),
        'p95Ms': ordered[round((len(ms) - 1) * .95)],
        'p99Ms': ordered[round((len(ms) - 1) * .99)],
        'SDKRealPresentRate': real_rate, 'SDKGeneratedPresentRate': generated_rate,
        'SDKCountEquivalentTotal': total_rate, 'physicalDisplayFPSMeasured': False,
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('samples', type=Path)
    parser.add_argument('--fg', choices=('off', 'on'), required=True)
    args = parser.parse_args()
    rows = [json.loads(line) for line in args.samples.read_text().splitlines()]
    print(json.dumps(analyze(rows, args.fg == 'on'), indent=2))
