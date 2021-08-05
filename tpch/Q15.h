// suppkey, s_name, s_address, s_phone, total_revenue
using Q15_elt = tuple<dkey_t, char *, char *, char *, double>;
using Q15_rtype = pbbs::sequence<Q15_elt>;

Q15_rtype Q15(maps m, const char *start_date) {
    delivery_map &dm = m.dm;
    delivery_map delivery_range = delivery_map::downTo(dm, Date(start_date));

    using ftype = double;
    using sr_type = pair<uint, float>;

    auto map_orderlines = [&](ol_map::E &ol) -> sr_type {
        return make_pair(ol.ol_suppkey, ol.ol_amount);
    };

    pbbs::sequence <sr_type> elts = flatten<sr_type>(delivery_range, map_orderlines);

    int max_supp_id = SUPPLIER_NUM + 1;

    auto get_index = [](sr_type &a) { return a.first; };
    auto get_val = [](sr_type &a) -> ftype { return (ftype) a.second; };

    pbbs::sequence <ftype> supp_sums =
            pbbs::collect_reduce(elts, get_index, get_val, pbbs::addm<ftype>(), max_supp_id);

    ftype max_revenue = pbbs::reduce(supp_sums, pbbs::maxm<ftype>());

    auto x = [&](size_t i) -> Q15_elt {
        Supplier &s = static_data.all_supp[i];
        return Q15_elt(i, s.su_name(), s.su_address(), s.su_phone(), supp_sums[i]);
    };
    auto is_in = [&](size_t i) -> bool {
        return supp_sums[i] >= max_revenue;
    };
    return pbbs::pack(pbbs::delayed_seq<Q15_elt>(max_supp_id, x),
                      pbbs::delayed_seq<bool>(max_supp_id, is_in));
}

double Q15time(maps m, bool verbose) {
    timer t;
    t.start();
    const char start[] = "2007-01-02";

    Q15_rtype result = Q15(m, start);
    double ret_tm = t.stop();
    cout << "Q15 : " << ret_tm << endl;

    if (verbose) {
        Q15_elt r = result[0];
        cout << "Q15:" << endl
             << get<0>(r) << ", "
             << get<1>(r) << ", "
             << get<2>(r) << ", "
             << get<3>(r) << ", "
             << get<4>(r) << endl;
    }
    return ret_tm;
}
