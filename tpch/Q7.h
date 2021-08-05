using pfloat = array<double, 6>;
using Q7_rtype = array<pfloat, 2>;

Q7_rtype Q7(maps m, size_t n1id, size_t n2id, const char* _start, const char* _end) {
    Date start = Date(_start);
    Date end = Date(_end);
    int start_year = start.year();
    timer t;
    t.start();
    supp_to_item_map spm = m.sim2;
    int max_supplier = (*(spm.previous(1000000000))).first;
    sequence <uchar> supplier_nation(max_supplier + 1);

    auto supplier_f = [&](supp_to_item_map::E &se, size_t i) -> void {
        Supplier &s = se.second.first;
        supplier_nation[s.su_suppkey] = s.su_nationkey;
    };

    supp_to_item_map::map_index(spm, supplier_f);

    using Add = Add_Nested_Array<Q7_rtype>;

    auto customer_f = [&](customer_map::E &ce) -> Q7_rtype{
        unsigned int customer_nation = ce.second.first.c_n_nationkey;
        order_map &omap = ce.second.second;
        if (customer_nation == n2id || customer_nation == n1id) {
            auto order_f = [&](order_map::E &oe) {
                ol_map &ol = oe.second.second;
                auto ol_f = [&](Q7_rtype &a, OrderLine &ol) {
                    Date delivery_date = ol.ol_delivery_d;
                    double rev = ol.ol_amount;
                    int year = delivery_date.year();
                    if (Date::greater(delivery_date, start) && Date::less(delivery_date, end)) {
                        if (customer_nation == n2id && supplier_nation[ol.ol_suppkey] == n1id) {
                            a[1][year-start_year] += rev;
                        }else if (customer_nation == n1id && supplier_nation[ol.ol_suppkey] == n2id) {
                            a[0][year-start_year] += rev;
                        }
                    }
                };
                return ol_map::semi_map_reduce(ol, ol_f, Add(), 2000);
            };
            return order_map::map_reduce(omap, order_f, Add());
        }
    };
    return customer_map::map_reduce(m.cm, customer_f, Add());
}

double Q7time(maps m, bool verbose) {
    timer t;
    t.start();
    //const char nation1[] = "FRANCE";
    //const char nation2[] = "GERMANY";
    unsigned int n1id = 55;
    unsigned int n2id = 115;
    const char start[] = "2007-01-02";
    const char end[] = "2012-01-02";

    Q7_rtype result = Q7(m, n1id, n2id, start, end);

    double ret_tm = t.stop();
    if (query_out) cout << "Q7 : " << ret_tm << endl;

    if (verbose) {
        cout << "Q7:" << endl
             << "Germany" << ", "
             << "CAMBODIA" << ", "
             << "2007, "
             << result[0][0] << endl;
    }
    return ret_tm;
}
