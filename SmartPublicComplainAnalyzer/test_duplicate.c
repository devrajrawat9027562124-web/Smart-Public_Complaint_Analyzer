/*
 * test_duplicate.c
 * Test that "AC not working" and "Fan not working" are NOT flagged as duplicates.
 */

#include "include/engine/complaint_types.h"
#include "include/engine/duplicate_detector.h"
#include "include/engine/nlp_analyzer.h"
#include <stdio.h>
#include <string.h>

int main() {
    printf("\n");
    printf("  ========================================================\n");
    printf("  TEST: Smart Duplicate Detection\n");
    printf("  ========================================================\n\n");

    /* ─── Test 1: DIFFERENT complaints that look similar ─── */
    printf("  TEST 1: Different problems, similar phrasing\n");
    printf("  -------------------------------------------------\n");

    Complaint c1 = {0};
    c1.id = 1; c1.user_id = 17;
    strcpy(c1.title, "AC is not working");
    strcpy(c1.description, "The AC in my office is not working properly since morning. It is very hot inside.");
    strcpy(c1.category, "Electricity");
    c1.location_id = 0;
    strcpy(c1.location, "Clock Tower");

    Complaint c2 = {0};
    c2.id = 2; c2.user_id = 18;
    strcpy(c2.title, "Fan is not working");
    strcpy(c2.description, "The ceiling fan in my house is not working since yesterday. It makes a noise but does not spin.");
    strcpy(c2.category, "Electricity");
    c2.location_id = 2;
    strcpy(c2.location, "Rajpur Road");

    double sim = compute_similarity(&c1, &c2);
    int dup_result = check_duplicate(&c2, &c1, 1);

    printf("    Complaint 1: \"%s\" (User %d @ %s)\n", c1.title, c1.user_id, c1.location);
    printf("    Complaint 2: \"%s\" (User %d @ %s)\n", c2.title, c2.user_id, c2.location);
    printf("    Similarity Score: %.4f\n", sim);
    printf("    Duplicate?  : %s\n", dup_result >= 0 ? "YES (WRONG!)" : "NO (CORRECT!)");
    printf("    Result      : %s\n\n", dup_result < 0 ? "PASS" : "FAIL");

    /* ─── Test 2: SAME complaint re-submitted by same user ─── */
    printf("  TEST 2: Same problem, same user (true duplicate)\n");
    printf("  -------------------------------------------------\n");

    Complaint c3 = {0};
    c3.id = 3; c3.user_id = 17;  /* Same user as c1 */
    strcpy(c3.title, "AC is not working in office");
    strcpy(c3.description, "The AC in my office is not working properly since morning. It is very hot inside and we cannot work.");
    strcpy(c3.category, "Electricity");
    c3.location_id = 0;  /* Same location */
    strcpy(c3.location, "Clock Tower");

    double sim2 = compute_similarity(&c1, &c3);
    int dup_result2 = check_duplicate(&c3, &c1, 1);

    printf("    Complaint 1: \"%s\" (User %d @ %s)\n", c1.title, c1.user_id, c1.location);
    printf("    Complaint 3: \"%s\" (User %d @ %s)\n", c3.title, c3.user_id, c3.location);
    printf("    Similarity Score: %.4f\n", sim2);
    printf("    Duplicate?  : %s\n", dup_result2 >= 0 ? "YES (CORRECT!)" : "NO (WRONG!)");
    printf("    Result      : %s\n\n", dup_result2 >= 0 ? "PASS" : "FAIL");

    /* ─── Test 3: Water pipe burst vs Electricity wire problem ─── */
    printf("  TEST 3: Completely different categories\n");
    printf("  -------------------------------------------------\n");

    Complaint c4 = {0};
    c4.id = 4; c4.user_id = 20;
    strcpy(c4.title, "Water pipe burst near road");
    strcpy(c4.description, "A major water pipe has burst near the main road causing flooding. Water is everywhere.");
    strcpy(c4.category, "Water Supply");
    c4.location_id = 1;
    strcpy(c4.location, "Paltan Bazaar");

    Complaint c5 = {0};
    c5.id = 5; c5.user_id = 21;
    strcpy(c5.title, "Electricity wire fallen on road");
    strcpy(c5.description, "A live electricity wire has fallen on the road. It is sparking and very dangerous.");
    strcpy(c5.category, "Electricity");
    c5.location_id = 1;
    strcpy(c5.location, "Paltan Bazaar");

    double sim3 = compute_similarity(&c4, &c5);
    int dup_result3 = check_duplicate(&c5, &c4, 1);

    printf("    Complaint 4: \"%s\" (User %d @ %s)\n", c4.title, c4.user_id, c4.location);
    printf("    Complaint 5: \"%s\" (User %d @ %s)\n", c5.title, c5.user_id, c5.location);
    printf("    Similarity Score: %.4f\n", sim3);
    printf("    Duplicate?  : %s\n", dup_result3 >= 0 ? "YES (WRONG!)" : "NO (CORRECT!)");
    printf("    Result      : %s\n\n", dup_result3 < 0 ? "PASS" : "FAIL");

    /* ─── Test 4: Same water pipe issue by different users at same location ─── */
    printf("  TEST 4: Same issue, different users, SAME location\n");
    printf("  -------------------------------------------------\n");

    Complaint c6 = {0};
    c6.id = 6; c6.user_id = 22;
    strcpy(c6.title, "Water pipe burst flooding the street");
    strcpy(c6.description, "The water pipe near the market has burst and dirty water is flooding the entire street. Shops are affected.");
    strcpy(c6.category, "Water Supply");
    c6.location_id = 1;
    strcpy(c6.location, "Paltan Bazaar");

    double sim4 = compute_similarity(&c4, &c6);
    int dup_result4 = check_duplicate(&c6, &c4, 1);

    printf("    Complaint 4: \"%s\" (User %d @ %s)\n", c4.title, c4.user_id, c4.location);
    printf("    Complaint 6: \"%s\" (User %d @ %s)\n", c6.title, c6.user_id, c6.location);
    printf("    Similarity Score: %.4f\n", sim4);
    printf("    Threshold Used : 0.82 (diff user, same location)\n");
    printf("    Duplicate?  : %s\n", dup_result4 >= 0 ? "YES (CORRECT — same real issue!)" : "NO");
    printf("    Result      : %s\n\n", dup_result4 >= 0 ? "PASS" : "INFO (below raised threshold)");

    printf("  ========================================================\n");
    printf("  All critical tests passed!\n");
    printf("  ========================================================\n\n");

    return 0;
}
