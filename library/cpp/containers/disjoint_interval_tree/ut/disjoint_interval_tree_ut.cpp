#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/containers/disjoint_interval_tree/disjoint_interval_tree.h>

Y_UNIT_TEST_SUITE(DisjointIntervalTreeTest) {
    Y_UNIT_TEST(GenericTest) {
        TDisjointIntervalTree<ui64> tree;
        tree.Insert(1);
        tree.Insert(50);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 2);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 2);

        tree.InsertInterval(10, 30);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 22);

        UNIT_ASSERT_VALUES_EQUAL(tree.Min(), 1);
        UNIT_ASSERT_VALUES_EQUAL(tree.Max(), 51);

        tree.Erase(20);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 4);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 21);

        tree.Clear();
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 0);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 0);
    }

    Y_UNIT_TEST(MergeIntervalsTest) {
        TDisjointIntervalTree<ui64> tree;
        tree.Insert(1);
        tree.Insert(2);

        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
        UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 2);

        auto begin = tree.begin();
        UNIT_ASSERT_VALUES_EQUAL(begin->first, 1);
        UNIT_ASSERT_VALUES_EQUAL(begin->second, 3);

        ++begin;
        UNIT_ASSERT_EQUAL(begin, tree.end());
    }

    Y_UNIT_TEST(EraseIntervalTest) {
        // 1. Remove from empty tree.
        {
            TDisjointIntervalTree<ui64> tree;

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(1, 3), 0);
        }

        // 2. No such interval in set.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(1, 3), 0);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(20, 30), 0);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);
        }

        // 3. Remove the whole tree.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(0, 100), 5);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 0);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 0);
            UNIT_ASSERT(tree.Empty());
        }

        // 4. Remove the whole tree with borders specified exactly as in tree.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(5, 10), 5);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 0);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 0);
            UNIT_ASSERT(tree.Empty());
        }

        // 5. Specify left border exactly as in existing interval.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            tree.InsertInterval(15, 20);
            tree.InsertInterval(25, 30);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(15, 100500), 10);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);
        }

        // 6. Specify left border somewhere in existing interval.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            tree.InsertInterval(15, 20);
            tree.InsertInterval(25, 30);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(16, 100500), 9);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 2);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 6);
        }

        // 7. Remove from the center of existing interval.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            tree.InsertInterval(15, 20);
            tree.InsertInterval(25, 30);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(17, 19), 2);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 4);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 13);

            UNIT_ASSERT(tree.Has(16));
            UNIT_ASSERT(tree.Has(19));
        }

        // 8. Remove from the center of the only existing interval.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(15, 20);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(17, 19), 2);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 2);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 3);

            UNIT_ASSERT(tree.Has(16));
            UNIT_ASSERT(tree.Has(19));
        }

        // 9. Specify borders between existing intervals.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            tree.InsertInterval(15, 20);
            tree.InsertInterval(25, 30);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(10, 15), 0);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(13, 15), 0);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(10, 13), 0);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);
        }

        // 10. Specify right border exactly as in existing interval.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            tree.InsertInterval(15, 20);
            tree.InsertInterval(25, 30);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(0, 20), 10);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 1);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 5);
        }

        // 11. Specify right border somewhere in existing interval.
        {
            TDisjointIntervalTree<ui64> tree;
            tree.InsertInterval(5, 10);
            tree.InsertInterval(15, 20);
            tree.InsertInterval(25, 30);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 3);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 15);

            UNIT_ASSERT_VALUES_EQUAL(tree.EraseInterval(2, 17), 7);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumIntervals(), 2);
            UNIT_ASSERT_VALUES_EQUAL(tree.GetNumElements(), 8);
        }
    }
}
