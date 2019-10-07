#include "multi_integro_diff.h"
#include "reflection_correction.h"
#include "preprocessing.h"
#include "normalization.h"
#include "test.h"
#include "occlusion_reflection_detec.h"
#include <chrono>
#include <ctime>
#include <string.h>
#include <vector>

using namespace cv;
using namespace std;

int run_Utiris(string db_path, string out_path);

int run_MICHE(string db_path, string out_path, string haar_casc);

int test();

/**
 * @brief creo le directory presenti in inp_path all'interno di out_path e ritorno i rispettivi path (ritorno out_vec che contiene tali path). N.B.: valido per Utiris
 * @param inp_path path alla directory di input (in cui si trova il DB, in questo caso Utiris)
 * @param out_path path alla directory di output in cui andranno a crearsi tante dirs quante sono in inp_path
 * @return vector<string> out_vec contenente i path alle singole directory create
 */
vector<string> create_dirs(string inp_path, string out_path);