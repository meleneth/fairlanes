"""Trace normalization must support both older Clang and Clang 22 reports."""
import importlib.util
from pathlib import Path
import unittest

script = Path(__file__).resolve().parents[2] / 'scripts/profile_compile_times.py'
spec = importlib.util.spec_from_file_location('compile_profile', script)
profile = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profile)


class TraceSpans(unittest.TestCase):
    def test_complete_span_retains_measured_duration(self):
        event = dict(ph='X', name='InstantiateClass', ts=100, dur=50)
        self.assertEqual(list(profile.timed_events([event])), [event])

    def test_async_source_span_retains_header_and_computes_duration(self):
        events = [dict(ph='b', pid=1, tid=2, cat='Source', id=0,
                       name='Source', ts=100, args={'detail': 'heavy.hpp'}),
                  dict(ph='e', pid=1, tid=2, cat='Source', id=0,
                       name='Source', ts=450)]
        spans = list(profile.timed_events(events))
        self.assertEqual(len(spans), 1)
        self.assertEqual(spans[0]['dur'], 350)
        self.assertEqual(spans[0]['args']['detail'], 'heavy.hpp')

    def test_reused_async_id_does_not_mix_successive_headers(self):
        events = [dict(ph='b', name='Source', id=0, ts=10),
                  dict(ph='e', name='Source', id=0, ts=40),
                  dict(ph='b', name='Source', id=0, ts=50),
                  dict(ph='e', name='Source', id=0, ts=120)]
        self.assertEqual([e['dur'] for e in profile.timed_events(events)], [30, 70])

    def test_metadata_is_not_a_measured_span(self):
        self.assertEqual(list(profile.timed_events([dict(ph='M', name='process_name')])), [])
