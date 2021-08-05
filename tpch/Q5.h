using Q5_elt = pair<uint, double>;
using Q5_rtype = sequence<Q5_elt>;

Q5_rtype Q5(maps m, char *start, uint qregion) {
    Date ostart = Date(start);
    customer_map cm = m.cm;

    constexpr int num_nations = 123;
    using T = array<double, num_nations>;

    auto customer_f = [&](T &a, customer_map::E &ce) -> void {
        uint nationid = ce.second.first.c_n_nationkey;
        if (static_data.all_nation[nationid].n_regionkey == qregion) {
            auto order_f = [&](order_map::E &oe) -> double {
                Order &ord = oe.second.first;
                if (Date::less(ord.o_entry_d, ostart))
                    return 0.0;
                auto ol_f = [&](ol_map::E &ol) -> double {
                    uint suppid = ol.ol_suppkey;
                    Supplier &suppV = static_data.all_supp[suppid];
                    if (suppV.su_nationkey != nationid) return 0.0;
                    else return ol.ol_amount;
                };
                ol_map ol = oe.second.second;
                return ol_map::map_reduce(ol, ol_f, Add<double>());
            };
            order_map om = ce.second.second;
            a[nationid] += order_map::map_reduce(om, order_f, Add<double>());
        }
    };

    T nat_sums =
            customer_map::semi_map_reduce(cm, customer_f, Add_Array<T>());

    // convert from array to sequence, and sort by revenue
    sequence <Q5_elt> r(num_nations, [&](size_t i) {
        return Q5_elt(i, nat_sums[i]);
    });
    auto less = [](Q5_elt a, Q5_elt b) -> bool { return a.second > b.second; };
    return pbbs::sample_sort(r, less);
    return Q5_rtype();
}


double Q5time(maps m, bool verbose) {
    timer t_q5;
    t_q5.start();
    char start[] = "2007-01-02";
    unsigned int qregion = 3;

    Q5_rtype result = Q5(m, start, qregion);

    double ret_tm = t_q5.stop();
    if (query_out) cout << "Q5 : " << ret_tm << endl;

    if (verbose) {
        Q5_elt r = result[0];
        cout << "Q5:" << endl
             << static_data.all_nation[r.first].name() << ", " << r.second << endl;
    }
    return ret_tm;
}
