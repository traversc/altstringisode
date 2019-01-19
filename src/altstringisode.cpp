#include <iostream>
#include "altrepisode.h"
#include <string>
#include <vector>

std::string getRandomStrings(int seed, int len = 30) {
  std::srand(static_cast<unsigned int>(seed));
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  std::string s(len, '\0');
  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  return s;
}

struct random_string_data {
  int N;
  random_string_data(int n) : N(n) {}
};

struct alt_random_string {
  static R_altrep_class_t class_t;
  
  // constructor function
  static SEXP Make(random_string_data* data, bool owner){
    SEXP xp = PROTECT(R_MakeExternalPtr(data, R_NilValue, R_NilValue));
    if (owner) {
      R_RegisterCFinalizerEx(xp, alt_random_string::Finalize, TRUE);
    }
    SEXP res = R_new_altrep(class_t, xp, R_NilValue);
    UNPROTECT(1);
    return res;
  }
  
  // finalizer for the external pointer
  static void Finalize(SEXP xp){
    delete static_cast<random_string_data*>(R_ExternalPtrAddr(xp));
  }
  
  // get the std::vector<string>* from the altrep object `x`
  static random_string_data* Ptr(SEXP x) {
    return static_cast<random_string_data*>(R_ExternalPtrAddr(R_altrep_data1(x)));
  }
  
  // same, but as a reference, for convenience
  static random_string_data& Get(SEXP vec) {
    return *Ptr(vec) ;
  }
  
  // ALTREP methods -------------------
  // The length of the object
  static R_xlen_t Length(SEXP vec){
    return Get(vec).N;
  }
  
  // What gets printed when .Internal(inspect()) is used
  static Rboolean Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)){
    Rprintf("random_string_data (len=%d, ptr=%p)\n", Length(x), Ptr(x));
    return TRUE;
  }
  
  // ALTVEC methods ------------------
  static SEXP Materialize(SEXP vec) {
    std::cout << "Materialize method called\n";
    SEXP data2 = R_altrep_data2(vec);
    if (data2 != R_NilValue) {
      return data2;
    }
    R_xlen_t n = Length(vec);
    data2 = PROTECT(Rf_allocVector(STRSXP, n));
    
    auto data1 = Get(vec);
    for (R_xlen_t i = 0; i < n; i++) {
      std::string mystring = getRandomStrings(i);
      SET_STRING_ELT(data2, i, Rf_mkCharLen(mystring.data(), mystring.size()) );
    }
    
    R_set_altrep_data2(vec, data2);
    UNPROTECT(1);
    return data2;
  }
  
  // The start of the data, i.e. the underlying string array from the random_string_data
  //
  // This is guaranteed to never allocate (in the R sense)
  static const void* Dataptr_or_null(SEXP vec) {
    SEXP data2 = R_altrep_data2(vec);
    if (data2 == R_NilValue) return nullptr;
    return STDVEC_DATAPTR(data2);
  }
  
  // same in this case, writeable is ignored
  static void* Dataptr(SEXP vec, Rboolean writeable) {
    return STDVEC_DATAPTR(Materialize(vec));
  }
  
  
  // ALTSTRING methods -----------------
  // the element at the index `i`
  // this does not do bounds checking because that's expensive, so
  // the caller must take care of that
  static SEXP string_Elt(SEXP vec, R_xlen_t i){
    std::cout << "string_Elt method called: " << i << "\n";
    std::string mystring = getRandomStrings(i);
    return Rf_mkCharLen(mystring.data(), mystring.size());
  }
  
  // -------- initialize the altrep class with the methods above
  static void Init(DllInfo* dll){
    class_t = R_make_altstring_class("alt_random_string", "altstringisode", dll);
    
    // altrep
    R_set_altrep_Length_method(class_t, Length);
    R_set_altrep_Inspect_method(class_t, Inspect);
    
    // altvec
    R_set_altvec_Dataptr_method(class_t, Dataptr);
    R_set_altvec_Dataptr_or_null_method(class_t, Dataptr_or_null);
    
    // altstring
    R_set_altstring_Elt_method(class_t, string_Elt);
  }
  
};

// static initialization of stdvec_double::class_t
R_altrep_class_t alt_random_string::class_t;

// Called the package is loaded (needs Rcpp 0.12.18.3)
// [[Rcpp::init]]
void init_stdvec_double(DllInfo* dll){
  alt_random_string::Init(dll);
}

// [[Rcpp::export]]
SEXP altrandomStrings(int N) {
  auto x = new random_string_data(N);
  return alt_random_string::Make(x, true);
}
