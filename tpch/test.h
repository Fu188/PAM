//
// Created by 17124 on 2021/8/11.
//

#ifndef PARTIAL_HTAP_TEST_H
#define PARTIAL_HTAP_TEST_H

#include <math.h>
#include <vector>
#include <map>
#include "readCSV.h"
#include "lineitem.h"
#include "pbbslib/get_time.h"
#include "pbbslib/parse_command_line.h"
#include "pbbslib/collect_reduce.h"
#include "pbbslib/random_shuffle.h"
#include "pbbslib/monoid.h"
#include "pam.h"
#include "utils.h"
#include "tables.h"
#include "queries.h"
#include "new_orders.h"

struct insert_form {
    string col_name;
    string value;
};

void Vec2Map(vector <insert_form> v, std::map<string, string> &row_map);
double insertOrder(int64_t txn_id, vector <insert_form> v);
double insertOrderline(int64_t txn_id, vector<insert_form> v);
double updateMap(string table_name, string pkeyColName, string pkeyVal, string colId, string value);
double op(int op_type, int64_t txnid, string table_name, string pkey_col_name, string pkey_val, string col_name, string value);

pair<double, double> geo_mean(double *x, int round);
pair<double, double> arith_mean(double *x, int round);

void output_res(double **tm, int round, int queries);

void test(string data_directory, bool verbose);

#endif //PARTIAL_HTAP_TEST_H
