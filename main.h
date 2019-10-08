#include "iris_localization.h"
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

/**
 * @file
 * @brief main della segmentazione dell'iride
 */

/**
 * @brief segmentazione per Utiris
 * @param db_path path assoluto del database
 * @param out_path path assoluto in cui verrà salvato l'output dell'algoritmo
 */
int run_Utiris(string db_path, string out_path);


/**
 * @brief segmentazione per MICHE
 * @param db_path path assoluto del database
 * @param out_path path assoluto in cui verrà salvato l'output dell'algoritmo
 * @param haar_casc path assoluto del file .xml contenente il classificatore per Viola-Jones
 */
int run_MICHE(string db_path, string out_path, string haar_casc);

int test();

/**
 * @brief creo le directory presenti in inp_path all'interno di out_path e ritorno i rispettivi path (ritorno out_vec che contiene tali path). N.B.: valido per Utiris
 * @param inp_path path alla directory di input (in cui si trova il DB, in questo caso Utiris)
 * @param out_path path alla directory di output in cui andranno a crearsi tante dirs quante sono in inp_path
 * @return vector<string> out_vec contenente i path alle singole directory create
 */
vector<string> create_dirs(string inp_path, string out_path);