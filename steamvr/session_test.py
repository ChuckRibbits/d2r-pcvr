"""Real subprocess checks for cleanup ownership and normal exit behavior."""
import subprocess
import sys
import unittest
from session import run


class SessionTest(unittest.TestCase):
    def test_normal_exit(self):
        self.assertEqual(run([sys.executable, '-c', 'raise SystemExit(7)'], poll=.02), 7)

    def test_game_exit_cleans_lingering_wrapper(self):
        # argv[0] mimics Wine's Windows game executable without launching D2R.
        code = ('import subprocess,sys,time; '
                'p=subprocess.Popen([r"S:\\game\\D2RLoader.exe", "-c", "import time;time.sleep(.3)"], '
                'executable=sys.executable); p.wait(); time.sleep(30)')
        unrelated = subprocess.Popen([sys.executable, '-c', 'import time;time.sleep(30)'])
        try:
            self.assertEqual(run([sys.executable, '-c', code], poll=.02, grace=.1), 0)
            self.assertIsNone(unrelated.poll())
        finally:
            unrelated.terminate()
            unrelated.wait(timeout=3)

    def test_no_game_does_not_force_cleanup(self):
        self.assertEqual(run([sys.executable, '-c', 'import time;time.sleep(.2);raise SystemExit(9)'], poll=.02, grace=.02), 9)


if __name__ == '__main__':
    unittest.main()
