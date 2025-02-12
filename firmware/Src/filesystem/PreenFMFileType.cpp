/*
 * Copyright 2020 Xavier Hosxe
 *
 * Author: Xavier Hosxe (xavier <dot> hosxe (at) g m a i l <dot> com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PreenFMFileType.h"

__attribute__((section(".ram_d2b"))) char storageBuffer[PROPERTY_FILE_SIZE];
__attribute__((section(".ram_d2b"))) static FIL file;

PreenFMFileType::PreenFMFileType() {
    isInitialized_ = false;
    numberOfFiles_ = 0;
    // init error file
    char empty[] = "<Empty>\0";
    for (int k = 0; k < 8; k++) {
        errorFile_.name[k] = empty[k];
    }
    errorFile_.fileType = FILE_EMPTY;
}

PreenFMFileType::~PreenFMFileType() {
    // TODO Auto-generated destructor stub
}

const char* PreenFMFileType::getFileName(FILE_ENUM file) {
    switch (file) {
        case DEFAULT_MIXER:
            return DEFAULT_MIXER_NAME;
        case DEFAULT_SEQUENCE:
            return DEFAULT_SEQUENCE_NAME;
        case PROPERTIES:
            return PROPERTIES_NAME;
        case MIDI_CONTROLLER_STATE:
            return MIDI_CONTROLLER_STATE_NAME;
    }
}

const char* PreenFMFileType::getFullName(const char *fileName) {
    const char *folderName = getFolderName();
    int pos = 0;
    int cpt = 0;
    int len = fsu_->strlen(folderName);
    for (int k = 0; k < len && cpt++ < 24; k++) {
        fullName_[pos++] = folderName[k];
    }
    fullName_[pos++] = '/';
    cpt = 0;
    len = fsu_->strlen(fileName);
    for (int k = 0; k < len && cpt++ < 14; k++) {
        fullName_[pos++] = fileName[k];
    }
    fullName_[pos] = 0;
    return fullName_;
}

int PreenFMFileType::remove(FILE_ENUM file) {
    f_unlink(getFileName(file));
}

int PreenFMFileType::load(FILE_ENUM file, int seek, void *bytes, int size) {
    return load(getFileName(file), seek, bytes, size);
}

int PreenFMFileType::load(const char *fileName, int seek, void *bytes, int size) {

    FRESULT fatFSResult = f_open(&file, fileName, FA_READ);
    if (seek == 0 && size == 0) {
        // read the full file
        size = file.obj.objsize;
    }
    int toReturn = 0;
    if (fatFSResult == FR_OK) {

        if (seek != 0) {
            f_lseek(&file, seek);
        }
        if (fatFSResult == FR_OK) {
            UINT byteRead;
            fatFSResult = f_read(&file, bytes, size, &byteRead);
            if (fatFSResult == FR_OK && byteRead == size) {
                toReturn = byteRead;
            }
        }
        fatFSResult = f_close(&file);
        if (fatFSResult != FR_OK) {
            toReturn = 0;
        }
    }
    return toReturn;
}

FIL PreenFMFileType::createFile(const char *fileName) {
    FRESULT fatFSResult = f_open(&file, fileName, FA_OPEN_ALWAYS | FA_WRITE);
    if (fatFSResult != FR_OK) {
        file.err = 1;
    }
    return file;
}

bool PreenFMFileType::closeFile(FIL &file) {
    FRESULT fatFSResult = f_close(&file);
    if (fatFSResult != FR_OK) {
        return false;
    }
    return true;
}

int PreenFMFileType::saveData(FIL &file, void *bytes, uint32_t size) {
    int toReturn = 0;
    UINT byteWritten;
    FRESULT fatFSResult = f_write(&file, bytes, size, &byteWritten);
    if (fatFSResult == FR_OK && byteWritten == size) {
        toReturn = byteWritten;
    }
    return toReturn;

}

int PreenFMFileType::save(FILE_ENUM file, int seek, void *bytes, int size) {
    return save(getFileName(file), seek, bytes, size);
}

int PreenFMFileType::save(const char *fileName, int seek, void *bytes, int size) {
    FRESULT fatFSResult = f_open(&file, fileName, FA_OPEN_ALWAYS | FA_WRITE);
    int toReturn = 0;
    if (fatFSResult == FR_OK) {

        if (seek != 0) {
            f_lseek(&file, seek);
        }
        if (fatFSResult == FR_OK) {
            UINT byteWritten;
            fatFSResult = f_write(&file, bytes, size, &byteWritten);
            if (fatFSResult == FR_OK && byteWritten == size) {
                toReturn = byteWritten;
            }
        }
        fatFSResult = f_close(&file);
        if (fatFSResult != FR_OK) {
            toReturn = 0;
        }
    }
    return toReturn;
}

int PreenFMFileType::checkSize(FILE_ENUM file) {
    return checkSize(getFileName(file));
}

int PreenFMFileType::checkSize(const char *fileName) {
    FRESULT fatFSResult = f_open(&file, fileName, FA_READ);
    int toReturn = -1;
    if (fatFSResult == FR_OK) {
        toReturn = f_size(&file);
        f_close(&file);
    }
    return toReturn;
}

// Init bank
int PreenFMFileType::initFiles() {

    numberOfFiles_ = 0;
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    for (int k = 0; k < numberOfFilesMax_; k++) {
        myFiles_[k].fileType = FILE_EMPTY;
    }

    res = f_opendir(&dir, getFolderName());
    if (res == FR_OK) {

        while (numberOfFiles_ < numberOfFilesMax_) {
            bool beforePoint = true;
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) {
                break;
            }
            if (!(fno.fattrib & AM_DIR)) {
                if (isCorrectFile((char*) fno.fname, fno.fsize)) {
                    for (int k = 0; k < 13; k++) {
                        if (fno.fname[k] == ' ') {
                            myFiles_[numberOfFiles_].name[k] = '_';
                        } else if (fno.fname[k] == 0 && beforePoint) {
                            myFiles_[numberOfFiles_].name[k] = '~';
                        } else {
                            if (fno.fname[k] == '.') {
                                beforePoint = false;
                            }
                            myFiles_[numberOfFiles_].name[k] = fno.fname[k];
                        }
                    }
                    if (fno.fname[0] == '_' || isReadOnly(&myFiles_[numberOfFiles_])) {
                        myFiles_[numberOfFiles_].fileType = FILE_READ_ONLY;
                    } else {
                        myFiles_[numberOfFiles_].fileType = FILE_OK;
                    }
                    numberOfFiles_++;
                }
            }
        }
        f_closedir(&dir);
    }

    sortFiles(myFiles_, numberOfFiles_);
    isInitialized_ = true;
    return numberOfFiles_;
}

int PreenFMFileType::readNextFile(struct PFM3File *bank) {
    return 0;
}

const struct PFM3File* PreenFMFileType::getFile(int fileNumber) {
    if (!isInitialized_) {
        initFiles();
    }
    if (fileNumber < 0 || fileNumber >= numberOfFiles_) {
        return &errorFile_;
    }
    return &myFiles_[fileNumber];
}

int PreenFMFileType::getFileIndex(const char *name) {
    if (!isInitialized_) {
        initFiles();
    }
    for (int k = 0; k < numberOfFiles_; k++) {
        if (fsu_->str_cmp(name, myFiles_[k].name) == 0) {
            return k;
        }
    }
    return -1;
}

int PreenFMFileType::getFileIndex(const struct PFM3File *file) {
    if (!isInitialized_) {
        initFiles();
    }
    for (int k = 0; k < numberOfFiles_; k++) {
        if (&myFiles_[k] == file) {
            return k;
        }
    }
    return -1;
}

int PreenFMFileType::renameFile(const struct PFM3File *bank, const char *newName) {
    isInitialized_ = false;

    // getFullName(bank->name) -> getFullName(newName)
    char fullNewBankName[40];
    const char *fullNameTmp = getFullName(newName);
    for (int k = 0; k < 40; k++) {
        fullNewBankName[k] = fullNameTmp[k];
    }
    return (int) f_rename(getFullName(bank->name), fullNewBankName);
}

void PreenFMFileType::swapFiles(struct PFM3File *bankFiles, int i, int j) {
    if (i == j) {
        return;
    }
    struct PFM3File tmp = bankFiles[i];
    bankFiles[i] = bankFiles[j];
    bankFiles[j] = tmp;
}

void PreenFMFileType::sortFiles(struct PFM3File *bankFiles, int numberOfFiles) {
    for (int i = 0; i < numberOfFiles - 1; i++) {
        int minBank = i;
        for (int j = i + 1; j < numberOfFiles; j++) {
            if (fsu_->strcmp(bankFiles[minBank].name, bankFiles[j].name) > 0) {
                minBank = j;
            }
        }
        swapFiles(bankFiles, i, minBank);
    }
}

/*******************************************************
 *  engine2.pfm3Version : Set the version we save.
 *  Used by convertFlashToParams to make sure what we're reading from the SD card
 *
 *  > version 1.0
 *    engine2.spread and engine2.detune available
 *    engine1.monoPoly has a new value : 2 for unison
 *
 *********************************************************/
void PreenFMFileType::convertParamsToFlash(const struct OneSynthParams *params, struct FlashSynthParams *memory, bool saveArp) {
    // Engines line
    fsu_->copyFloat((float*) &params->engine1, (float*) &memory->engine1, 4);
    fsu_->copyFloat((float*) &params->engine2, (float*) &memory->engine2, 4);

    // VERSION : 1.0
    memory->engine2.pfm3Version = 1.0f;


    if (saveArp) {
        fsu_->copyFloat((float*) &params->engineArp1, (float*) &memory->engineArp1, 4 * 2);
        memory->engineArpUserPatterns = params->engineArpUserPatterns;
    } else {
        memory->engineArp1.clock = 0;
        memory->engineArp1.BPM = 90;
        memory->engineArp1.octave = 1;
        memory->engineArp2.pattern = 2;
        memory->engineArp2.division = 12;
        memory->engineArp2.duration = 14;
        memory->engineArp2.latche = 0;
        for (int p = 0; p < ARRAY_SIZE(memory->engineArpUserPatterns.patterns); ++p)
            memory->engineArpUserPatterns.patterns[p] = 0;
    }

    memory->flashEngineIm1.modulationIndex1 = params->engineIm1.modulationIndex1;
    memory->flashEngineIm1.modulationIndex2 = params->engineIm1.modulationIndex2;
    memory->flashEngineIm1.modulationIndex3 = params->engineIm2.modulationIndex3;
    memory->flashEngineIm1.modulationIndex4 = params->engineIm2.modulationIndex4;
    memory->flashEngineIm2.modulationIndex5 = params->engineIm3.modulationIndex5;
    memory->flashEngineIm2.modulationIndex6 = params->engineIm3.modulationIndex6;
    memory->flashEngineIm2.notUsed1 = 0.0f;
    memory->flashEngineIm2.notUsed2 = 0.0f;

    memory->flashEngineVeloIm1.modulationIndexVelo1 = params->engineIm1.modulationIndexVelo1;
    memory->flashEngineVeloIm1.modulationIndexVelo2 = params->engineIm1.modulationIndexVelo2;
    memory->flashEngineVeloIm1.modulationIndexVelo3 = params->engineIm2.modulationIndexVelo3;
    memory->flashEngineVeloIm1.modulationIndexVelo4 = params->engineIm2.modulationIndexVelo4;
    memory->flashEngineVeloIm2.modulationIndexVelo5 = params->engineIm3.modulationIndexVelo5;
    memory->flashEngineVeloIm2.modulationIndexVelo6 = params->engineIm3.modulationIndexVelo6;
    memory->flashEngineVeloIm2.notUsed1 = 0.0f;
    memory->flashEngineVeloIm2.notUsed1 = 0.0f;

    fsu_->copyFloat((float*) &params->engineMix1, (float*) &memory->engineMix1, 4 * 3);
    fsu_->copyFloat((float*) &params->effect, (float*) &memory->effect, 4);
    fsu_->copyFloat((float*) &params->osc1, (float*) &memory->osc1, 4 * 6);
    fsu_->copyFloat((float*) &params->env1a, (float*) &memory->env1a, 4 * 6 * 2);
    fsu_->copyFloat((float*) &params->matrixRowState1, (float*) &memory->matrixRowState1, 4 * 12);
    fsu_->copyFloat((float*) &params->lfoOsc1, (float*) &memory->lfoOsc1, 4 * 3);
    fsu_->copyFloat((float*) &params->lfoEnv1, (float*) &memory->lfoEnv1, 4);
    fsu_->copyFloat((float*) &params->lfoEnv2, (float*) &memory->lfoEnv2, 4);
    fsu_->copyFloat((float*) &params->lfoSeq1, (float*) &memory->lfoSeq1, 4 * 2);

    fsu_->copyFloat((float*) &params->midiNote1Curve, (float*) &memory->midiNote1Curve, 4);
    fsu_->copyFloat((float*) &params->midiNote2Curve, (float*) &memory->midiNote2Curve, 4);
    fsu_->copyFloat((float*) &params->lfoPhases, (float*) &memory->lfoPhases, 4);


    for (int s = 0; s < 16; s++) {
        memory->lfoSteps1.steps[s] = params->lfoSteps1.steps[s];
        memory->lfoSteps2.steps[s] = params->lfoSteps2.steps[s];
    }
    for (int s = 0; s < 13; s++) {
        memory->presetName[s] = params->presetName[s];
    }
}

void PreenFMFileType::convertFlashToParams(const struct FlashSynthParams *memory, struct OneSynthParams *params, bool loadArp) {
    // First engine line
    fsu_->copyFloat((float*) &memory->engine1, (float*) &params->engine1, 4);
    fsu_->copyFloat((float*) &memory->engine2, (float*) &params->engine2, 4);

    if (loadArp) {
        fsu_->copyFloat((float*) &memory->engineArp1, (float*) &params->engineArp1, 4 * 2);
        params->engineArpUserPatterns = memory->engineArpUserPatterns;
    }

    params->engineIm1.modulationIndex1 = memory->flashEngineIm1.modulationIndex1;
    params->engineIm1.modulationIndex2 = memory->flashEngineIm1.modulationIndex2;
    params->engineIm2.modulationIndex3 = memory->flashEngineIm1.modulationIndex3;
    params->engineIm2.modulationIndex4 = memory->flashEngineIm1.modulationIndex4;
    params->engineIm3.modulationIndex5 = memory->flashEngineIm2.modulationIndex5;
    params->engineIm3.modulationIndex6 = memory->flashEngineIm2.modulationIndex6;
    params->engineIm1.modulationIndexVelo1 = memory->flashEngineVeloIm1.modulationIndexVelo1;
    params->engineIm1.modulationIndexVelo2 = memory->flashEngineVeloIm1.modulationIndexVelo2;
    params->engineIm2.modulationIndexVelo3 = memory->flashEngineVeloIm1.modulationIndexVelo3;
    params->engineIm2.modulationIndexVelo4 = memory->flashEngineVeloIm1.modulationIndexVelo4;
    params->engineIm3.modulationIndexVelo5 = memory->flashEngineVeloIm2.modulationIndexVelo5;
    params->engineIm3.modulationIndexVelo6 = memory->flashEngineVeloIm2.modulationIndexVelo6;

    fsu_->copyFloat((float*) &memory->engineMix1, (float*) &params->engineMix1, 4 * 3);
    fsu_->copyFloat((float*) &memory->effect, (float*) &params->effect, 4);
    fsu_->copyFloat((float*) &memory->osc1, (float*) &params->osc1, 4 * 6);
    fsu_->copyFloat((float*) &memory->env1a, (float*) &params->env1a, 4 * 6 * 2);
    fsu_->copyFloat((float*) &memory->matrixRowState1, (float*) &params->matrixRowState1, 4 * 12);
    fsu_->copyFloat((float*) &memory->lfoOsc1, (float*) &params->lfoOsc1, 4 * 3);
    fsu_->copyFloat((float*) &memory->lfoEnv1, (float*) &params->lfoEnv1, 4);
    fsu_->copyFloat((float*) &memory->lfoEnv2, (float*) &params->lfoEnv2, 4);
    fsu_->copyFloat((float*) &memory->lfoSeq1, (float*) &params->lfoSeq1, 4 * 2);

    fsu_->copyFloat((float*) &memory->lfoPhases, (float*) &params->lfoPhases, 4);
    fsu_->copyFloat((float*) &memory->midiNote1Curve, (float*) &params->midiNote1Curve, 4);
    fsu_->copyFloat((float*) &memory->midiNote2Curve, (float*) &params->midiNote2Curve, 4);

    for (int s = 0; s < 16; s++) {
        params->lfoSteps1.steps[s] = memory->lfoSteps1.steps[s];
        params->lfoSteps2.steps[s] = memory->lfoSteps2.steps[s];
    }
    for (int s = 0; s < 13; s++) {
        params->presetName[s] = memory->presetName[s];
    }

    params->performance1.perf1 = 0.0f;
    params->performance1.perf2 = 0.0f;
    params->performance1.perf3 = 0.0f;
    params->performance1.perf4 = 0.0f;

    // Initialized not initialize params in memory
    if (params->engineArp1.BPM < 10) {
        params->engineArp1.clock = 0;
        params->engineArp1.BPM = 90;
        params->engineArp1.octave = 1;
        params->engineArp2.pattern = 2;
        params->engineArp2.division = 12;
        params->engineArp2.duration = 14;
        params->engineArp2.latche = 0;
    }

    if (params->effect.type == 0.0f && params->effect.param1 == 0.0f && params->effect.param2 == 0.0f && params->effect.param3 == 0.0f) {
        params->effect.param1 = 0.5f;
        params->effect.param2 = 0.5f;
        params->effect.param3 = 1.0f;
    }

    if (params->midiNote1Curve.breakNote == 0.0f && params->midiNote1Curve.curveAfter == 0.0f && params->midiNote1Curve.curveBefore == 0.0f) {
        // Default compatibility value
        // FLAT 0 +Lin
        params->midiNote1Curve.curveAfter = 1;
    }
    if (params->midiNote2Curve.breakNote == 0.0f && params->midiNote2Curve.curveAfter == 0.0f && params->midiNote2Curve.curveBefore == 0.0f) {
        // Default compatibility value
        // FLAT 0 +Lin
        params->midiNote2Curve.curveBefore = 4;
        params->midiNote2Curve.curveAfter = 1;
        params->midiNote2Curve.breakNote = 60;
    }

    // Fixe poly Mono depending on pfm3Version :
    uint32_t version = (uint32_t)(params->engine2.pfm3Version + .1f);
    if (version == 0) {
        // map to new parameters
        // Compatible with pfm2 bank
        // in pfm2 bank playMode was the number of voice
        if (params->engine1.playMode > 2.0f
            || (params->engine1.playMode == 2.0f && params->engine2.unisonSpread == 0.0f && params->engine2.unisonDetune == 0.0f)) {
            params->engine1.playMode = PLAY_MODE_POLY;
        }
        // Old preset, we set unisonSpread and unisonDetune different from 0
        if (params->engine2.unisonSpread == 0.0f && params->engine2.unisonDetune == 0.0f) {
            params->engine2.unisonSpread = 0.5f;
            params->engine2.unisonDetune = 0.12f;
        }
        // Map glide speed to new glideSpeed and glideType
        if (params->engine1.glideSpeed > 0.0f) {
            params->engine2.glideType = GLIDE_TYPE_OVERLAP;
        } else {
            params->engine2.glideType = GLIDE_TYPE_OFF;
        }
    }
}

int PreenFMFileType::bankBaseLength(const char *bankName) {
    int k;
    for (k = 0; k < 8 && bankName[k] != 0 && bankName[k] != '.'; k++)
        ;
    return k;
}

bool PreenFMFileType::nameExists(const char *bankName) {
    int nameLength = bankBaseLength(bankName);
    for (int b = 0; getFile(b)->fileType != FILE_EMPTY && b < numberOfFilesMax_; b++) {
        const struct PFM3File *pfmb = getFile(b);
        if (nameLength != bankBaseLength(pfmb->name)) {
            continue;
        }
        bool sameName = true;
        for (int n = 0; n < nameLength && sameName; n++) {
            // Case insensitive...
            char c1 = bankName[n];
            char c2 = pfmb->name[n];
            if (c1 >= 'a' && c1 <= 'z') {
                c1 = 'A' + c1 - 'a';
            }
            if (c2 >= 'a' && c2 <= 'z') {
                c2 = 'A' + c2 - 'a';
            }
            if (c1 != c2) {
                sameName = false;
            }
        }
        if (sameName) {
            return true;
        }
    }
    return false;
}

const struct PFM3File* PreenFMFileType::addEmptyFile(const char *fileName) {
    int k;
    for (k = 0; myFiles_[k].fileType != FILE_EMPTY && k < numberOfFilesMax_; k++)
        ;

    if (k == numberOfFilesMax_) {
        // NO EMPTY file
        return 0;
    }
    myFiles_[k].fileType = FILE_OK;
    for (int n = 0; n < 12; n++) {
        myFiles_[k].name[n] = fileName[n];
    }
    isInitialized_ = false;
    return &myFiles_[k];
}

