#ifndef NDB_H
#define NDB_H

namespace NDB {

    enum Result{
        Ok, 
        NotImplemented, 
        InitError, 
        SymbolError, 
        NullError, 
        ForbiddenError, 
        ParamError, 
        ConnError
    };
    
}

#endif // NDB_H