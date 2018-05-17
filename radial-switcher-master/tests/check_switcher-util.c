#include <stdlib.h>
#include <check.h>
#include <stdio.h>

#include <compiz.h>

#include "../src/switcher-util.h"

WindowTree *tree;
WindowList *list;

CompWindow *a, *b, *c, *d;
WindowCmpProc *compare;

static Bool testCompare (CompWindow *a, CompWindow *b) 
{
    return a == b;
}

void setup ()
{
    int i;
    list = newWindowList ();
    tree = newWindowTree ();
    compare = malloc (sizeof (WindowCmpProc) * 2);
    compare[0] = &testCompare;
    compare[1] = NULL;
    a = 0x5;
    b = 0x4;
    c = 0x4;
    d = 0x4;
}

void teardown ()
{
    destroyWindowList (list);
    free (compare);
}

START_TEST (test_listCreate)
{
    fail_unless (list->nWindows == 0,
		 "Window list count not set correctly on creation");
    fail_unless (list->w != NULL,
		 "Window array not allocated successfully");
    fail_unless (list->windowsSize == 32,
		 "Window array size set incorrectly");
}
END_TEST

START_TEST (test_treeCreate)
{
    fail_unless (tree->windows->nWindows == 0 && tree->windows->w != NULL && \
		 tree->windows->windowsSize == 32,
		 "Failed to initialise WindowList on WindowTree creation");
    fail_unless (tree->children != NULL,
		 "Children array not allocated on creation of WindowTree");
    fail_unless (tree->childrenSize == 4,
		 "Size of childrenArray not set correctly");
}
END_TEST

START_TEST (test_addToList)
{
    fail_unless (addWindowToList (list, a),
		 "addWindowToList returned FALSE");

    fail_unless (list->nWindows == 1,
		 "Window list size not updated correctly");
    fail_unless (list->w[0] == a,
		 "Wrong window added to list");
}
END_TEST

START_TEST (test_addMultipleToList)
{
    CompWindow *array[500];
    int i;
    
    for (i = 0; i < 500; ++i)
	array[i] = (CompWindow *)i;

    for (i = 0; i < 500; ++i)
	fail_unless (addWindowToList (list, array[i]),
		     "addWindowToList returned FALSE");

    for (i = 0; i < 500; ++i)
	fail_unless (list->w[i] == (CompWindow *)i,
		     "Incorrect window inserted in list");
		     
    fail_unless(list->nWindows == 500,
		"Number of windows not updated correctly");      
}
END_TEST


START_TEST (test_addToTree)
{
    fail_unless (addWindowToTree (tree, a, compare),
		 "addWindowToTree returned FALSE");
    
    fail_unless (tree->windows->nWindows == 1,
		 "Window not added to tree");
    fail_unless (tree->windows->w[0] == a,
		 "Incorrect window added to tree");
}
END_TEST

START_TEST (test_addMultipleToFlatTree)
{
    CompWindow *array[500];
    int i;
    
    for (i = 0; i < 500; ++i)
	array[i] = (CompWindow *)i;

    for (i = 0; i < 500; ++i)
	fail_unless (addWindowToTree (tree, array[i], compare),
		     "addWindowToTree returned FALSE");

    for (i = 0; i < 500; ++i)
	fail_unless (tree->windows->w[i] == (CompWindow *)i,
		     "Incorrect window inserted in tree");
		     
    fail_unless(tree->windows->nWindows == 500,
		"Number of windows not updated correctly");      
}
END_TEST

START_TEST (test_addGroupedWindows)
{
    a = 0x5;
    b = 0x3;
    c = 0x3;
    
    fail_unless (addWindowToTree (tree, a, compare),
		 "addWindowToTree returned FALSE");
    fail_unless (addWindowToTree (tree, b, compare),
		 "addWindowToTree returned FALSE");
    fail_unless (addWindowToTree (tree, c, compare),
		 "addWindowToTree returned FALSE");

    fail_unless (tree->windows->w[0] == a && tree->windows->w[1] == b,
		 "Top level windows failed to add properly");
    fail_unless (tree->children[1]->windows->w[0] == c,
		 "Child window failed to add properly");
}
END_TEST

START_TEST (test_addManyGroupedWindows)
{
    CompWindow *array[500];
    int i,j;
    
    for (i = 0; i < 500; ++i)
	array[i] = (CompWindow *) (i % 10);
    
    for (i = 0; i < 500; ++i)
	fail_unless (addWindowToTree (tree, array[i], compare),
		     "addWindowToTree returned FALSE");

    fail_unless (tree->windows->nWindows == 10,
		 "Incorrect number of groups created");
}
END_TEST

START_TEST (test_correctTreeStructure)
{
    CompWindow *array[500];
    int i,j;
    
    for (i = 0; i < 500; ++i)
	array[i] = (CompWindow *) (i % 10);
    
    for (i = 0; i < 500; ++i)
	fail_unless (addWindowToTree (tree, array[i], compare),
		     "addWindowToTree returned FALSE");

    for (i = 0; i < tree->windows->nWindows; ++i)
    {
	fail_unless (tree->windows->w[i] == (CompWindow *) i,
		     "Top-level window added in wrong order");
	fail_unless (tree->children[i]->windows->nWindows == 49,
		     "Incorrect number of subwindows");
	for (j = 0 ; j < 50-1; ++j)
	{
	    fail_unless (tree->children[i]->windows->w[j] == (CompWindow *)i,
			 "Incorrect window in subgroup");
	}
    }
}
END_TEST

Suite *
switcher_util_suite (void)
{
    Suite *s = suite_create ("Switcher Util");

    /* Core test case */
    TCase *tc_core = tcase_create ("Core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    tcase_add_test (tc_core, test_listCreate);
    tcase_add_test (tc_core, test_treeCreate);
    suite_add_tcase (s, tc_core);

    /* Add windows to tree List cases */
    TCase *tc_addToList = tcase_create ("Add To List");
    tcase_add_checked_fixture (tc_addToList, setup, teardown);
    tcase_add_test (tc_addToList, test_addToList);
    tcase_add_test (tc_addToList, test_addMultipleToList);
    suite_add_tcase (s, tc_addToList);
  
    /* Add windows to tree test cases */
    TCase *tc_addToTree = tcase_create ("Add To Tree");
    tcase_add_checked_fixture (tc_addToTree, setup, teardown);
    tcase_add_test (tc_addToTree, test_addToTree);
    tcase_add_test (tc_addToTree, test_addMultipleToFlatTree);
    tcase_add_test (tc_addToTree, test_addGroupedWindows);
    tcase_add_test (tc_addToTree, test_addManyGroupedWindows);
    tcase_add_test (tc_addToTree, test_correctTreeStructure);
    suite_add_tcase (s, tc_addToTree);

    return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = switcher_util_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
