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
        ConnError,
        TypeError,
        VolumeError
    };

}

#endif // NDB_H
