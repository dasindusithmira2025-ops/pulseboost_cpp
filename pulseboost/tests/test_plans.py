import unittest

from core.cognition.plans import features_for_plan, normalize_plan, plan_at_least


class PlanTests(unittest.TestCase):
    def test_unknown_plan_normalizes_to_free(self):
        self.assertEqual(normalize_plan("unknown"), "free")

    def test_free_features_are_limited(self):
        features = features_for_plan("free")
        self.assertEqual(features.history_hours, 1)
        self.assertFalse(features.predictions)
        self.assertFalse(features.audit_log)

    def test_plan_order_helper(self):
        self.assertTrue(plan_at_least("team", "pro"))
        self.assertFalse(plan_at_least("free", "pro"))


if __name__ == "__main__":
    unittest.main()
