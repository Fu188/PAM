using Q17_rtype = double;

Q17_rtype Q17(maps m, const char *data) {
    using int_pair = pair<int, int>;
    using AddI = Add<int>;
    using AddD = Add<double>;
    using AddP = Add_Pair<AddI, AddI>;
    int typelen = strlen(data);

    auto item_f = [&](item_to_supp_map::E &e) -> double {
        Item &p = e.second.first;
        char *t = p.i_data();
        // if correct part brand and container
        if (strcmp(t + strlen(t) - typelen, data) == 0) {

            // calculate average quantity
            auto supp_f1 = [&](item_supp_and_orderline_map::E &e) -> int_pair {
                ol_map &lm = e.second.second;
                auto ol_f1 = [&](ol_map::E &ol) -> int {
                    return ol.ol_quantity;
                };
                int v = ol_map::map_reduce(lm, ol_f1, AddI());
                return int_pair(v, lm.size());
            };
            item_supp_and_orderline_map &simap = e.second.second;
            int_pair x = item_supp_and_orderline_map::map_reduce(simap, supp_f1, AddP());
            double threshold = ((double) x.first) / (double) x.second;

            // calculate total extended price if < 20% of average quantity
            auto supp_f2 = [&](item_supp_and_orderline_map::E &e) {
                ol_map &lm = e.second.second;
                auto ol_f2 = [=](ol_map::E &l) -> double {
                    return (((double) l.ol_quantity) < threshold) ? l.ol_amount : 0.0;
                };
                return ol_map::map_reduce(lm, ol_f2, AddD());
            };
            return item_supp_and_orderline_map::map_reduce(simap, supp_f2, AddD());
        } else return 0.0;
    };

    double total = item_to_supp_map::map_reduce(m.ism2, item_f, AddD());
    return total / 2.0;
}

double Q17time(maps m, bool verbose) {
    timer t;
    t.start();
    const char data[] = "b";

    Q17_rtype result = Q17(m, data);

    double ret_tm = t.stop();
    cout << "Q17 : " << ret_tm << endl;

    if (verbose) {
        cout << "Q17:" << endl << result << endl;
    }

    return ret_tm;
}
