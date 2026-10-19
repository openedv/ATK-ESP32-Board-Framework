import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).with_name("discover_matrix.py")
SPEC = importlib.util.spec_from_file_location("discover_matrix", SCRIPT_PATH)
discover_matrix = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(discover_matrix)


class DiscoverBoardsTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.repo_root = Path(self.temp_dir.name)
        self.original_repo_root = discover_matrix.REPO_ROOT
        discover_matrix.REPO_ROOT = self.repo_root

    def tearDown(self):
        discover_matrix.REPO_ROOT = self.original_repo_root
        self.temp_dir.cleanup()

    def _add_board(self, relative_dir: str, name: str):
        board_dir = self.repo_root / relative_dir
        board_dir.mkdir(parents=True)
        (board_dir / "board_info.yaml").write_text(
            f"board: {name}\nchip: esp32s3\n", encoding="utf-8"
        )

    def test_discovers_board_directories_up_to_three_levels_deep(self):
        self._add_board("root_board", "root_board")
        self._add_board("audio/speaker_board", "speaker_board")
        self._add_board("display/round/screen_board", "screen_board")
        self._add_board("archived/old/unused/ignored_board", "ignored_board")

        boards = discover_matrix._discover_boards()

        self.assertEqual(
            boards,
            [
                {"board": "speaker_board", "target": "esp32s3"},
                {"board": "screen_board", "target": "esp32s3"},
                {"board": "root_board", "target": "esp32s3"},
            ],
        )
