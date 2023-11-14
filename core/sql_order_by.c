#include <assert.h>
#include <vector>
#include <bits/stdc++.h>
#include "qep.h"
#include "sql_order_by.h"
#include "SqlMexprIntf.h"

bool 
qep_collect_dtypes_for_sorting (qep_struct_t *qep) {

    int i;
    Dtype *dtype;
    qp_col_t *sqp_col;
    std::vector <Dtype *> *cVector;

    if (qep->orderby.column_name[0] == '\0') return false;

    cVector = new std::vector<Dtype *>();

    for (i = 0; i < qep->select.n; i++) {

        sqp_col = qep->select.sel_colmns[i];
        
        dtype = sqp_col->computed_value ?  \
                    sqp_col->computed_value : \
                    sql_column_get_aggregated_value (sqp_col);

        assert(dtype);
        cVector->push_back(dtype);

        if (sqp_col->computed_value) {
            sqp_col->computed_value = NULL;
        }
        else {
            sql_column_set_aggregated_value (sqp_col, NULL);
            sql_destroy_aggregator(sqp_col);
        }
    }

    qep->orderby.pVector.push_back (cVector);
    return true;
}

class DtypeVectorComparisonFunction {
    
    private:
    qep_struct_t *qep;

    public:

    DtypeVectorComparisonFunction(qep_struct_t *qep) {
        
        this->qep = qep;
    }

    bool 
    operator()(std::vector <Dtype *> *v1, std::vector <Dtype *> *v2) {

        int index = qep->orderby.orderby_col_select_index;
        
        Dtype *dtype1 = v1->at(index);
        Dtype *dtype2 = v2->at(index);

        if (qep->orderby.asc) {
            return Dtype_less_than_operator (dtype1, dtype2);
        }
        else {
            return !Dtype_less_than_operator (dtype1, dtype2);
        }

    }
};

void 
qep_orderby_sort (qep_struct_t *qep) {

    if (qep->orderby.column_name[0] == '\0') return;

    sort(qep->orderby.pVector.begin(), 
             qep->orderby.pVector.end(),
             DtypeVectorComparisonFunction (qep)); 

}

bool
qep_order_by_reassign_select_columns (qep_struct_t *qep) {

    int i;
    qp_col_t *sqp_col;

    std::vector < Dtype *> *cVector;

    if (qep->orderby.column_name[0] == '\0') return false;
    
    if (qep->orderby.iterator_index == qep->orderby.pVector.size()) return false;

    cVector =  qep->orderby.pVector.at(qep->orderby.iterator_index);

    for (i = 0; i < qep->select.n; i++) {

        sqp_col = qep->select.sel_colmns[i];
        
        if (sqp_col->agg_fn != SQL_AGG_FN_NONE) {
            
            assert(!sqp_col->aggregator);
            assert (cVector->at(i));

            sqp_col->aggregator = sql_get_aggregator ( 
                                        sqp_col->agg_fn, SQL_DTYPE_MAX);
            sql_column_set_aggregated_value (sqp_col, cVector->at(i));
        }
        else {
            sqp_col->computed_value =  cVector->at(i);
        }
    }

    qep->orderby.pVector[qep->orderby.iterator_index] = NULL;
    delete cVector;
    qep->orderby.iterator_index++;

    return true;
}   
