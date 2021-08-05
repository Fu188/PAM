// mkt_share for two years: 1995 and 1996
using Q8_rtype = array<float, 6>;

Q8_rtype Q8(maps m, const size_t regionk, const size_t nationk, const char *type) {
    const char start[] = "2007-01-02";
    const char end[] = "2012-01-02";

    int start_year = 2007;
    using float_array = array<float, 6>;
    using sums = array<float_array, 2>;
    using Add = Add_Nested_Array<sums>;

    int typelen = strlen(type);

    // for each part
    auto item_f = [&](item_to_supp_map::E &pe) -> sums {
        item_supp_and_orderline_map &supps = pe.second.second;
        char *t = pe.second.first.i_data();

        // check that part is correct type
        // about 1/150 will pass this test
        if (strcmp(t + strlen(t) - typelen, type) == 0) {

            auto supp_f = [&](item_supp_and_orderline_map::E &se) -> sums {
                ol_map &orderlines = se.second.second;
                //Supplier supp = se.second.first;
                uint suppkey = se.second.first.s_su_suppkey;
                Supplier &supp = static_data.all_supp[suppkey];

                // for each lineitem of the part
                auto orderline_f = [&](sums &a, ol_map::E &ol) {
                    float rev = 0.0;

                    // find the order from lineitem
                    Order o = (*(m.om.find(ol.ol_pkey))).first;

                    // check that in correct date range
                    // about 2/7 (of the 1/150)
                    Date d = o.o_entry_d;
                    int year = d.year();
                    if (Date::less(Date(start), d) && Date::less(d, Date(end))) {

                        // find the customer from order
                        Customer c = (*m.cm.find(o.o_pkey)).first;

                        // check that customer from correct region
                        if (static_data.all_nation[c.c_n_nationkey].n_regionkey == regionk) {
                            rev = ol.ol_amount;
                            if (supp.su_nationkey == nationk) {
                                a[0][year-start_year] += rev;
                            }
                            a[1][year-start_year] += rev;
                        }
                    }
                };
                return ol_map::semi_map_reduce(orderlines, orderline_f, Add(), 2000);
            };
            return item_supp_and_orderline_map::map_reduce(supps, supp_f, Add());
        }
    };

    sums r = item_to_supp_map::map_reduce(m.ism2, item_f, Add());
    Q8_rtype res;
    for (int i=0; i<6; i++) {
        res[i] = r[0][i] / r[1][i];
    }
    return res;
}

double Q8time(maps m, bool verbose) {
    timer t;
    t.start();
    //const char region[] = "AMERICA";
    const int regionk = 3;
    //const char nation[] = "BRAZIL";
    const int nationk = 55;
    const char type[] = "b";

    Q8_rtype result = Q8(m, regionk, nationk, type);

    double ret_tm = t.stop();
    cout << "Q8 : " << ret_tm << endl;

    if (verbose) {
        for (int i = 0; i < 6; i++) {
            cout << "Q8:" << endl << 2007+i << ", " << result[i] << endl;
        }
    }
    return ret_tm;
}

