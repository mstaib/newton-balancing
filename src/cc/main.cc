#include "matrixBalancing.h"
#include "MarketIO.h"
#include <unistd.h>
#include <chrono>

using namespace std;
using namespace Eigen;
using namespace std::chrono;

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool flag_in = false;
  bool flag_out = false;
  bool flag_stat = false;
  bool flag_sparse = false;
  bool do_newton = true;
  bool do_sinkhorn = false;
  bool do_bnewt = false;
  double rep_max = 1e+06;
  double error_tol = 1e-05;
  char *input_file = NULL;
  char *output_file = NULL;
  char *stat_file = NULL;

  // get arguments
  char opt;
  while ((opt = getopt(argc, argv, "i:o:t:pe:r:vnsb")) != -1) {
    switch (opt) {
    case 'i': input_file = optarg; flag_in = true; break;
    case 'o': output_file = optarg; flag_out = true; break;
    case 't': stat_file = optarg; flag_stat = true; break;
    case 'p': flag_sparse = true; break;
    case 'e': error_tol = pow(10, -1 * atof(optarg)); break;
    case 'r': rep_max = pow(10, atof(optarg)); break;
    case 'v': verbose = true; break;
    case 'n': do_newton = true; do_sinkhorn = false; do_bnewt = false; break;
    case 's': do_newton = false; do_sinkhorn = true; do_bnewt = false; break;
    case 'b': do_newton = false; do_sinkhorn = false; do_bnewt = true; break;
    }
  }

  if (!flag_in) {
    cerr << "> ERROR: Input file (-i [input_file]) is missing!" << endl;
    exit(1);
  }
  ofstream sfs;
  if (flag_stat) {
    sfs.open(stat_file);
  }

  cout << "> Reading a database file \"" << input_file << "\" ... " << flush;
  MatrixXd X_org;
  if (flag_sparse) {
    SparseMatrix<double> mat;
    string filename(input_file);
    bool ch = loadMarket(mat, filename);
    X_org = MatrixXd(mat);
  } else {
    ifstream ifs(input_file);
    readFromCSV(X_org, ifs);
    ifs.close();
  }
  cout << "end" << endl << flush;
  cout << "  Information:" << endl << flush;
  cout << "  Number of (rows, cols): (" << X_org.rows() << ", " << X_org.cols() << ")" << endl << flush;

  // check whether an input matrix is nonnegative
  for (Int i = 0; i < X_org.rows(); ++i) {
    for (Int j = 0; j < X_org.cols(); ++j) {
      if (X_org(i, j) < 0.0) {
	cout << "Negative value exists!" << endl;
	exit(1);
      }
    }
  }

  if (flag_stat) {
    sfs << "Number_of_rows:\t" << X_org.rows() << endl;
    sfs << "Number_of_cols:\t" << X_org.cols() << endl;
  }

  cout << "> Removing rows and columns with all zeros ... " << flush;
  vector<Int> idx_row;
  for (Int i = 0; i < X_org.rows(); ++i) {
    if (X_org.row(i).sum() > EPSILON) idx_row.push_back(i);
  }
  vector<Int> idx_col;
  for (Int j = 0; j < X_org.cols(); ++j) {
    if (X_org.col(j).sum() > EPSILON) idx_col.push_back(j);
  }
  MatrixXd X = MatrixXd::Zero(idx_row.size(), idx_col.size());
  for (Int i = 0; i < idx_row.size(); ++i) {
    for (Int j = 0; j < idx_col.size(); ++j) {
      X(i, j) = X_org(idx_row[i], idx_col[j]);
    }
  }
  cout << "end" << endl << flush;
  cout << "  Number of (rows, cols) after removing zero rows and cols: (" << X.rows() << ", " << X.cols() << ")" << endl << flush;
  if (flag_stat) {
    sfs << "Number_of_rows_after_zero_removal:\t" << X.rows() << endl;
    sfs << "Number_of_cols_after_zero_removal:\t" << X.cols() << endl;
  }

  Int type = 1;
  if (do_newton) {
    cout << "> Start Newton balancing algortihm:" << endl << flush;
    type = 1;
  } else if (do_sinkhorn) {
    cout << "> Start the Sinkhorn-Knopp balancing algorithm:" << endl << flush;
    type = 2;
  } else if (do_bnewt) {
    cout << "> Start the BNEWT algorithm:" << endl << flush;
    type = 3;
  }
  // run matrix balancing
  auto ts = system_clock::now();
  double step = MatrixBalancing(X, error_tol, rep_max, verbose, type);
  auto te = system_clock::now();
  auto dur = te - ts;
  cout << "  Number of iterations: " << step << endl;
  cout << "  Running time:         " << duration_cast<microseconds>(dur).count() << " [microsec]" << endl;
  if (flag_out) {
    const static IOFormat CSVFormat(StreamPrecision, DontAlignCols, ",", "\n");
    ofstream ofs(output_file);
    ofs << X.format(CSVFormat);
    ofs.close();
  }
  if (flag_stat) {
    sfs << "Number_of_iterations:\t" << step << endl;
    sfs << "Running_time_(microsec):\t" << duration_cast<microseconds>(dur).count() << endl;
    sfs.close();
  }
}
