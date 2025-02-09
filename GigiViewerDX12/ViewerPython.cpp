///////////////////////////////////////////////////////////////////////////////
//         Gigi Rapid Graphics Prototyping and Code Generation Suite         //
//        Copyright (c) 2024 Electronic Arts Inc. All rights reserved.       //
///////////////////////////////////////////////////////////////////////////////

// clang-format off
#include <vector>
#include "ViewerPython.h"
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#define PY_SSIZE_T_CLEAN
#ifdef __unix
  #ifdef _DEBUG
    #undef _DEBUG
    #include <Python.h>
    #include <tupleobject.h>
    #include <bytesobject.h>
    #define _DEBUG
  #else
    #include <Python.h>
    #include <tupleobject.h>
    #include <bytesobject.h>
  #endif
#else
  #ifdef _DEBUG
    #undef _DEBUG
    #include <python.h>
    #include <tupleobject.h>
    #include <bytesobject.h>
    #define _DEBUG 1
  #else
    #include <python.h>
    #include <tupleobject.h>
    #include <bytesobject.h>
  #endif
#endif
// clang-format on

// external functions
void PyInit_GigiArray();
struct _object* PyCreate_GigiArray(const GigiArray& array);


static PythonInterface* g_interface = nullptr;

static std::vector<std::wstring> g_argvText;
static std::vector<wchar_t*> g_argv;

static bool FileExists(const char* fileName)
{
    FILE* file = nullptr;
    fopen_s(&file, fileName, "rb");
    if (!file)
        return false;

    fclose(file);
    return true;
}

static PyObject* Python_LoadGG(PyObject* self, PyObject* args)
{
    const char* fileName = nullptr;
    if (!PyArg_ParseTuple(args, "s:Python_LoadGG", &fileName))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    // If the filename is relative, try relative to the python script first
    bool success = false;
    std::filesystem::path fileNamePath = fileName;
    if (fileNamePath.is_relative())
    {
        fileNamePath = std::filesystem::weakly_canonical(std::filesystem::path(g_interface->m_scriptLocation).remove_filename() / fileNamePath);
        if (FileExists(fileNamePath.string().c_str()))
            success = g_interface->LoadGG(fileNamePath.string().c_str());
    }

    // If the path wasn't relative, or it wasn't found relative to the python script,
    // try it as given.  If it was relative but not found, this will look for it relative
    // to the viewer exe file location.
    if (!success)
        success = g_interface->LoadGG(fileName);

    PyObject* ret = PyBool_FromLong(success ? 1 : 0);
    Py_INCREF(ret);
    return ret;
}

static PyObject* Python_Exit(PyObject* self, PyObject* args)
{
    int value = 0;
    if (!PyArg_ParseTuple(args, "|i:Python_Exit", &value))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->RequestExit(value);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetHideUI(PyObject* self, PyObject* args)
{
    int value = 1;
    if (!PyArg_ParseTuple(args, "p:Python_SetHideUI", &value))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetHideUI(value != 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetVSync(PyObject* self, PyObject* args)
{
    int value = 1;
    if (!PyArg_ParseTuple(args, "p:Python_SetVSync", &value))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetVSync(value != 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetSyncInterval(PyObject* self, PyObject* args)
{
    int value = 1;
    if (!PyArg_ParseTuple(args, "i:Python_SetSyncInterval", &value))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetSyncInterval(value);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetStablePowerState(PyObject* self, PyObject* args)
{
    int value = 1;
    if (!PyArg_ParseTuple(args, "p:Python_SetStablePowerState", &value))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetStablePowerState(value != 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetProfilingMode(PyObject* self, PyObject* args)
{
    int value = 1;
    if (!PyArg_ParseTuple(args, "p:Python_SetProfilingMode", &value))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetProfilingMode(value != 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetVariable(PyObject* self, PyObject* args)
{
    const char* variableName = nullptr;
    const char* variableValue = nullptr;
    if (!PyArg_ParseTuple(args, "ss:Python_SetVariable", &variableName, &variableValue))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    if (!g_interface->SetVariable(variableName, variableValue))
        return PyErr_Format(PyExc_TypeError, "Could not set variable \"%s\" in " __FUNCTION__ "()", variableName);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_GetScriptPath(PyObject* self, PyObject* args)
{
    std::string ret = std::filesystem::path(g_interface->GetScriptLocation()).remove_filename().string();
    return Py_BuildValue("s", ret.c_str());
}

static PyObject* Python_GetScriptFileName(PyObject* self, PyObject* args)
{
    std::string ret = std::filesystem::path(g_interface->GetScriptLocation()).filename().string();
    return Py_BuildValue("s", ret.c_str());
}

static PyObject* Python_GetVariable(PyObject* self, PyObject* args)
{
    const char* variableName = nullptr;
    if (!PyArg_ParseTuple(args, "s:Python_GetVariable", &variableName))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    std::string variableValue;
    if (!g_interface->GetVariable(variableName, variableValue))
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return Py_BuildValue("s", variableValue.c_str());
}

static PyObject* Python_DisableGGUserSave(PyObject* self, PyObject* args)
{
    int value = 1;
    if (!PyArg_ParseTuple(args, "p:Python_DisableGGUserSave", &value))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetDisableGGUserSave(value != 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetWantReadback(PyObject* self, PyObject* args)
{
    const char* viewableResourceName = nullptr;
    int wantsReadback = 1;
    int arrayIndex = 0;
    int mipIndex = 0;
    if (!PyArg_ParseTuple(args, "s|pii:Python_SetWantReadback", &viewableResourceName, &wantsReadback, &arrayIndex, &mipIndex))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetWantReadback(viewableResourceName, wantsReadback != 0, arrayIndex, mipIndex);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_Readback(PyObject* self, PyObject* args)
{
    const char* viewableResourceName = nullptr;
    if (!PyArg_ParseTuple(args, "s:Python_Readback", &viewableResourceName))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    // Get the data
    GigiArray data;
    bool success = g_interface->Readback(viewableResourceName, data);

    // return the result
    PyObject* ret = PyTuple_New(2);
    PyTuple_SetItem(ret, 0, PyCreate_GigiArray(data));
    PyTuple_SetItem(ret, 1, PyBool_FromLong(success ? 1 : 0));
    Py_INCREF(ret);
    return ret;
}

static PyObject* Python_RunTechnique(PyObject* self, PyObject* args)
{
    int runCount = 1;
    if (!PyArg_ParseTuple(args, "|i:Python_RunTechnique", &runCount))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->RunTechnique(runCount);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_Log(PyObject* self, PyObject* args)
{
    const char* severity = nullptr;
    const char* message = nullptr;
    if (!PyArg_ParseTuple(args, "ss:Python_Log", &severity, &message))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    LogLevel logLevel = LogLevel::Info;
    if (!_stricmp(severity, "Info"))
        logLevel = LogLevel::Info;
    else if (!_stricmp(severity, "Warn"))
        logLevel = LogLevel::Warn;
    else if (!_stricmp(severity, "Error"))
        logLevel = LogLevel::Error;

    g_interface->Log(logLevel, "Python_Log(): %s", message);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_Print(PyObject* self, PyObject* args)
{
    const char* message = nullptr;
    if (!PyArg_ParseTuple(args, "s:Python_Print", &message))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->Log(LogLevel::Info, "Python: %s", message);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_Warn(PyObject* self, PyObject* args)
{
    const char* message = nullptr;
    if (!PyArg_ParseTuple(args, "s:Python_Print", &message))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->Log(LogLevel::Warn, "Python: %s", message);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_Error(PyObject* self, PyObject* args)
{
    const char* message = nullptr;
    if (!PyArg_ParseTuple(args, "s:Python_Print", &message))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->Log(LogLevel::Error, "Python: %s", message);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetFrameIndex(PyObject* self, PyObject* args)
{
    int frameIndex = 0;
    if (!PyArg_ParseTuple(args, "i:Python_SetFrameIndex", &frameIndex))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetFrameIndex(frameIndex);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_WaitOnGPU(PyObject* self, PyObject* args)
{
    g_interface->WaitOnGPU();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_Pause(PyObject* self, PyObject* args)
{
    int pause = 1;
    if (!PyArg_ParseTuple(args, "p:Python_Pause", &pause))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->Pause(pause != 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_PixCaptureNextFrames(PyObject* self, PyObject* args)
{
    const char* fileName = nullptr;
    int frameCount = 1;
    if (!PyArg_ParseTuple(args, "s|i:Python_PixCaptureNextFrames", &fileName, &frameCount))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->PixCaptureNextFrames(fileName, frameCount);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedBufferFile(PyObject* self, PyObject* args)
{
    const char* bufferName = nullptr;
    const char* fileName = nullptr;
    if (!PyArg_ParseTuple(args, "ss:Python_SetImportedBufferFile", &bufferName, &fileName))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedBufferFile(bufferName, fileName);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedBufferStruct(PyObject* self, PyObject* args)
{
    const char* bufferName = nullptr;
    const char* structName = nullptr;
    if (!PyArg_ParseTuple(args, "ss:Python_SetImportedBufferStruct", &bufferName, &structName))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedBufferStruct(bufferName, structName);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedBufferType(PyObject* self, PyObject* args)
{
    const char* bufferName = nullptr;
    int type = 0;
    if (!PyArg_ParseTuple(args, "si:Python_SetImportedBufferStruct", &bufferName, &type))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedBufferType(bufferName, (DataFieldType)type);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedBufferCount(PyObject* self, PyObject* args)
{
    const char* bufferName = nullptr;
    int count = 0;
    if (!PyArg_ParseTuple(args, "si:Python_SetImportedBufferCount", &bufferName, &count))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedBufferCount(bufferName, count);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedBufferCSVHeaderRow(PyObject* self, PyObject* args)
{
    const char* bufferName = nullptr;
    int CSVHeaderRow = 0;
    if (!PyArg_ParseTuple(args, "sp:Python_SetImportedBufferCSVHeaderRow", &bufferName, &CSVHeaderRow))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedBufferCSVHeaderRow(bufferName, CSVHeaderRow);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureFile(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    const char* fileName = nullptr;
    if (!PyArg_ParseTuple(args, "ss:Python_SetImportedTextureFile", &textureName, &fileName))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureFile(textureName, fileName);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureMakeMips(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    int makeMips = 1;
    if (!PyArg_ParseTuple(args, "sp:Python_SetImportedTextureMakeMips", &textureName, &makeMips))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureMakeMips(textureName, makeMips);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureSourceIsSRGB(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    int sourceIsSRGB = 1;
    if (!PyArg_ParseTuple(args, "sp:Python_SetImportedTextureSourceIsSRGB", &textureName, &sourceIsSRGB))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureSourceIsSRGB(textureName, sourceIsSRGB);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureFormat(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    int textureFormat = 0;
    if (!PyArg_ParseTuple(args, "si:Python_SetImportedTextureFormat", &textureName, &textureFormat))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureFormat(textureName, textureFormat);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureColor(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "sffff:Python_SetImportedTextureColor", &textureName, &r, &g, &b, &a))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureColor(textureName, r, g, b, a);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureSize(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    int x, y, z;
    if (!PyArg_ParseTuple(args, "siii:Python_SetImportedTextureSize", &textureName, &x, &y, &z))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureSize(textureName, x, y, z);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureBinaryType(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    int type;
    if (!PyArg_ParseTuple(args, "si:Python_SetImportedTextureBinaryType", &textureName, &type))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureBinaryType(textureName, (GGUserFile_ImportedTexture_BinaryType)type);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureBinarySize(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    int x, y, z;
    if (!PyArg_ParseTuple(args, "siii:Python_SetImportedTextureBinarySize", &textureName, &x, &y, &z))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureBinarySize(textureName, x, y, z);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetImportedTextureBinaryChannels(PyObject* self, PyObject* args)
{
    const char* textureName = nullptr;
    int channels;
    if (!PyArg_ParseTuple(args, "si:Python_SetImportedTextureBinarySize", &textureName, &channels))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetImportedTextureBinaryChannels(textureName, channels);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetFrameDeltaTime(PyObject* self, PyObject* args)
{
    float deltaTime = 0.0f;
    if (!PyArg_ParseTuple(args, "f:Python_SetFrameDeltaTime", &deltaTime))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetFrameDeltaTime(deltaTime);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetCameraPos(PyObject* self, PyObject* args)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (!PyArg_ParseTuple(args, "fff:Python_SetCameraPos", &x, &y, &z))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetCameraPos(x, y, z);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_SetCameraAltitudeAzimuth(PyObject* self, PyObject* args)
{
    float altitude = 0.0f;
    float azimuth = 0.0f;
    if (!PyArg_ParseTuple(args, "ff:SetCameraAltitudeAzimuth", &altitude, &azimuth))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->SetCameraAltitudeAzimuth(altitude, azimuth);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_GetCameraPos(PyObject* self, PyObject* args)
{
    float x, y, z;
    g_interface->GetCameraPos(x, y, z);

    PyObject* ret = PyTuple_New(3);
    PyTuple_SetItem(ret, 0, PyFloat_FromDouble(x));
    PyTuple_SetItem(ret, 1, PyFloat_FromDouble(y));
    PyTuple_SetItem(ret, 2, PyFloat_FromDouble(z));
    Py_INCREF(ret);
    return ret;
}

static PyObject* Python_GetCameraAltitudeAzimuth(PyObject* self, PyObject* args)
{
    float altitude, azimuth;
    g_interface->GetCameraAltitudeAzimuth(altitude, azimuth);

    PyObject* ret = PyTuple_New(2);
    PyTuple_SetItem(ret, 0, PyFloat_FromDouble(altitude));
    PyTuple_SetItem(ret, 1, PyFloat_FromDouble(azimuth));
    Py_INCREF(ret);
    return ret;
}

static PyObject* Python_WriteGPUResource(PyObject* self, PyObject* args)
{
    const char* viewableResourceName = nullptr;
    PyObject* bytesObj = nullptr;
    int resourceIndex = 0;
    if (!PyArg_ParseTuple(args, "sS|i:Python_WriteGPUResource", &viewableResourceName, &bytesObj, &resourceIndex))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->WriteGPUResource(viewableResourceName, resourceIndex, PyBytes_AsString(bytesObj), PyBytes_Size(bytesObj));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_ForceEnableProfiling(PyObject* self, PyObject* args)
{
    int forceEnableProfiling = 1;
    if (!PyArg_ParseTuple(args, "|p:p", &forceEnableProfiling))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    g_interface->ForceEnableProfiling(forceEnableProfiling != 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Python_GetProfilingData(PyObject* self, PyObject* args)
{
    PyObject* ret = PyDict_New();
    std::vector<PythonInterface::ProfilingData> data = g_interface->GetProfilingData();
    for (const PythonInterface::ProfilingData& item : data)
    {
        PyObject* key = Py_BuildValue("s", item.label.c_str());
        PyObject* value = Py_BuildValue("ff", item.cpums, item.gpums);
        PyDict_SetItem(ret, key, value);
    }
    return ret;
}

static PyObject* Python_GGEnumValue(PyObject* self, PyObject* args)
{
    const char* enumName = nullptr;
    const char* enumLabel = nullptr;
    if (!PyArg_ParseTuple(args, "ss:Python_GGEnumValue", &enumName, &enumLabel))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    int value = g_interface->GGEnumValue(enumName, enumLabel);

    if (value == -1)
        return PyErr_Format(PyExc_TypeError, "Enum type not found " __FUNCTION__ "()");
    else if (value == -2)
        return PyErr_Format(PyExc_TypeError, "Enum value not found " __FUNCTION__ "()");

    return Py_BuildValue("i", value);
}

static PyObject* Python_GGEnumLabel(PyObject* self, PyObject* args)
{
    const char* enumName = nullptr;
    int enumValue = 0;
    if (!PyArg_ParseTuple(args, "si:Python_GGEnumLabel", &enumName, &enumValue))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    std::string label = g_interface->GGEnumLabel(enumName, enumValue);

    if (label.empty())
        return PyErr_Format(PyExc_TypeError, "Enum type not found or value invalid " __FUNCTION__ "()");

    return Py_BuildValue("s", label.c_str());
}

static PyObject* Python_GGEnumCount(PyObject* self, PyObject* args)
{
    const char* enumName = nullptr;
    if (!PyArg_ParseTuple(args, "s:Python_GGEnumCount", &enumName))
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()");

    int count = g_interface->GGEnumCount(enumName);

    if (count == -1)
        return PyErr_Format(PyExc_TypeError, "Enum type not found " __FUNCTION__ "()");

    return Py_BuildValue("i", count);
}

static PyObject* Python_GetGPUString(PyObject* self, PyObject* args)
{
    return Py_BuildValue("s", g_interface->GetGPUString().c_str());
}

// Enum FromString
#include "external/df_serialize/_common.h"
#define ENUM_BEGIN(_NAME, _DESCRIPTION) \
static PyObject* Python_##_NAME##FromString(PyObject* self, PyObject* args) \
{ \
    const char* enumStr = nullptr; \
    if (!PyArg_ParseTuple(args, "s:Python_" #_NAME "FromString", &enumStr)) \
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()"); \
    int enumValue = -1; \
    typedef _NAME ThisType;

#define ENUM_ITEM(_NAME, _DESCRIPTION) \
    if (!_stricmp(enumStr, #_NAME)) \
        enumValue = (int)ThisType::##_NAME;

#define ENUM_END() \
    PyObject* ret = PyLong_FromLong(enumValue); \
    Py_INCREF(ret); \
    return ret; \
}
// clang-format off
#include "external/df_serialize/_fillunsetdefines.h"
#include "Schemas/Schemas.h"
// clang-format on

// Enum ToString
#include "external/df_serialize/_common.h"
#define ENUM_BEGIN(_NAME, _DESCRIPTION) \
static PyObject* Python_##_NAME##ToString(PyObject* self, PyObject* args) \
{ \
    int enumValue = -1; \
    if (!PyArg_ParseTuple(args, "i:Python_" #_NAME "ToString", &enumValue)) \
        return PyErr_Format(PyExc_TypeError, "type error in " __FUNCTION__ "()"); \
    const char* enumStr = nullptr; \
    typedef _NAME ThisType; \
    switch((ThisType)enumValue) \
    {

#define ENUM_ITEM(_NAME, _DESCRIPTION) \
    case ThisType::_NAME: return Py_BuildValue("s", #_NAME);

#define ENUM_END() \
    } \
    return Py_BuildValue("s", "Invalid"); \
}
// clang-format off
#include "external/df_serialize/_fillunsetdefines.h"
#include "Schemas/Schemas.h"
// clang-format on

static void AddEnums(PyObject* module)
{
    char buffer[4096];

    #include "external/df_serialize/_common.h"

    #define ENUM_BEGIN(_NAME, _DESCRIPTION) \
    { \
        const char* enumName = #_NAME; \
        sprintf_s(buffer, "%s_FIRST", enumName); \
        PyModule_AddIntConstant(module, buffer, 0); \
        int enumValue = 0;

    #define ENUM_ITEM(_NAME, _DESCRIPTION) \
        sprintf_s(buffer, "%s_" #_NAME, enumName); \
        PyModule_AddIntConstant(module, buffer, enumValue); \
        enumValue++;

    #define ENUM_END() \
        sprintf_s(buffer, "%s_LAST", enumName); \
        PyModule_AddIntConstant(module, buffer, enumValue-1); \
        sprintf_s(buffer, "%s_COUNT", enumName); \
        PyModule_AddIntConstant(module, buffer, enumValue); \
    }
    // clang-format off
    #include "external/df_serialize/_fillunsetdefines.h"
    #include "Schemas/Schemas.h"
    // clang-format on
}

void PythonInit(PythonInterface* interface, int argc, char** argv, int firstPythonArgv)
{
    g_interface = interface;

    static PyMethodDef pythonModuleMethods[] =
    {
        {"LoadGG", Python_LoadGG, METH_VARARGS, "Loads a .gg file"},
        {"Exit", Python_Exit, METH_VARARGS, "Closes the application"},
        {"GetScriptPath", Python_GetScriptPath, METH_VARARGS, "Returns the path of the python script file ran, without a file name."},
        {"GetScriptFileName", Python_GetScriptFileName, METH_VARARGS, "Returns the file name of the python script file ran, without the path."},
        {"SetHideUI", Python_SetHideUI, METH_VARARGS, "Sets the HideUI flag"},
        {"SetVSync", Python_SetVSync, METH_VARARGS, "Sets the vsync flag"},
        {"SetSyncInterval", Python_SetSyncInterval, METH_VARARGS, "IDXGISwapChain::Present() parameter: Synchronize presentation after the nth vertical blank."},
        {"SetStablePowerState", Python_SetStablePowerState, METH_VARARGS, "Sets the stable power state flag"},
        {"SetProfilingMode", Python_SetProfilingMode, METH_VARARGS, "Turns on or off profiling mode. The viewer does extra resource copies and readback as part of regular operation. Profiling mode reduces that."},
        {"SetVariable", Python_SetVariable, METH_VARARGS, "Sets the value of a variable. String for variable name, string for variable value."},
        {"GetVariable", Python_GetVariable, METH_VARARGS, "Gets the value of a variable. String for variable name, returns variable value as a string."},
        {"DisableGGUserSave", Python_DisableGGUserSave, METH_VARARGS, "When the gg file is changed next or the application is closed, this will prevent the gguser file from being saved."},
        {"SetWantReadback", Python_SetWantReadback, METH_VARARGS, "Declare that you want to read back a resource, or specify that you no longer want it."},
        {"Readback", Python_Readback, METH_VARARGS, "Reads back a resource. Host.SetWantReadback must have been called prior to rendering."},
        {"RunTechnique", Python_RunTechnique, METH_VARARGS, "Runs the technique N times."},
        {"Log", Python_Log, METH_VARARGS, "Writes a message to the log."},
        {"Print", Python_Print, METH_VARARGS, "Writes a message to the log as INFO."},
        {"Warn", Python_Warn, METH_VARARGS, "Writes a message to the log as WARN."},
        {"Error", Python_Error, METH_VARARGS, "Writes a message to the log as ERROR."},
        {"SetFrameIndex", Python_SetFrameIndex, METH_VARARGS, "Sets the frame index."},
        {"WaitOnGPU", Python_WaitOnGPU, METH_VARARGS, "Waits until all GPU work is finished. Temporarily needed to make sure readback is done. Will improve readback interface later."},
        {"Pause", Python_Pause, METH_VARARGS, "Can pause or unpause viewer execution. Does not affect RunTechnique(), but useful for pausing then exiting the script to return user control at a specific frame to investigate."},
        {"PixCaptureNextFrames", Python_PixCaptureNextFrames, METH_VARARGS, "Do a pix capture for the next N frames rendered."},
        {"SetImportedBufferFile", Python_SetImportedBufferFile, METH_VARARGS, "Set the file name of an imported buffer"},
        {"SetImportedBufferStruct", Python_SetImportedBufferStruct, METH_VARARGS, "Set the struct type of an imported buffer"},
        {"SetImportedBufferType", Python_SetImportedBufferType, METH_VARARGS, "Set the type of an imported buffer"},
        {"SetImportedBufferCount", Python_SetImportedBufferCount, METH_VARARGS, "Set the count of an imported buffer"},
        {"SetImportedBufferCSVHeaderRow", Python_SetImportedBufferCSVHeaderRow, METH_VARARGS, "Set whether or not teh csv file has a header row"},
        {"SetImportedTextureFile", Python_SetImportedTextureFile, METH_VARARGS, "Set the file name of an imported texture"},
        {"SetImportedTextureSourceIsSRGB", Python_SetImportedTextureSourceIsSRGB, METH_VARARGS, "Set whether or not the file on disk is sRGB."},
        {"SetImportedTextureMakeMips", Python_SetImportedTextureMakeMips, METH_VARARGS, "Whether or not to make mips for the imported texture."},
        {"SetImportedTextureFormat", Python_SetImportedTextureFormat, METH_VARARGS, "Set the texture format of an imported texture"},
        {"SetImportedTextureColor", Python_SetImportedTextureColor, METH_VARARGS, "Set the color of an imported texture"},
        {"SetImportedTextureSize", Python_SetImportedTextureSize, METH_VARARGS, "Set the x,y,z dimensions an imported texture"},
        {"SetImportedTextureBinaryType", Python_SetImportedTextureBinaryType, METH_VARARGS, "Sets the data type of a binary imported texture"},
        {"SetImportedTextureBinarySize", Python_SetImportedTextureBinarySize, METH_VARARGS, "Sets the x,y,z dimensions of a binary imported texture"},
        {"SetImportedTextureBinaryChannels", Python_SetImportedTextureBinaryChannels, METH_VARARGS, "Sets the number of channels of a binary imported texture"},
        {"SetFrameDeltaTime", Python_SetFrameDeltaTime, METH_VARARGS, "Set the frame delta time, in seconds. Useful for recording videos by setting a fixed frame rate. Clear by setting to 0."},
        {"SetCameraPos", Python_SetCameraPos, METH_VARARGS, "Set the camera position"},
        {"SetCameraAltitudeAzimuth", Python_SetCameraAltitudeAzimuth, METH_VARARGS, "Set the camera altitude azimuth"},
        {"GetCameraPos", Python_GetCameraPos, METH_VARARGS, "Get camera position"},
        {"GetCameraAltitudeAzimuth", Python_GetCameraAltitudeAzimuth, METH_VARARGS, "Get the camera altitude azimuth"},
        {"WriteGPUResource", Python_WriteGPUResource, METH_VARARGS, "Writes a gpu resource during the next RunTechnique"},
        {"ForceEnableProfiling", Python_ForceEnableProfiling, METH_VARARGS, "If true, forces profiling on, even when the profiling window isn't being shown."},
        {"GetProfilingData", Python_GetProfilingData, METH_VARARGS, "Gets the profiling data from the last technique execution. Python_ForceEnableProfiling needs to be enabled. Time is in milliseconds. CPU time is first, GPU time is second."},
        {"GGEnumValue", Python_GGEnumValue, METH_VARARGS, "Gets the integer value of an enum defined in the loaded .gg file."},
        {"GGEnumLabel", Python_GGEnumLabel, METH_VARARGS, "Gets the string label of an enum defined in the loaded .gg file."},
        {"GGEnumCount", Python_GGEnumCount, METH_VARARGS, "Returns the number of enum values for an enum defined in the loaded .gg file."},
        {"GetGPUString", Python_GetGPUString, METH_VARARGS, "Returns the name of the gpu and driver version."},

        // Enum FromString and ToString functions
        #include "external/df_serialize/_common.h"
        #define ENUM_BEGIN(_NAME, _DESCRIPTION) \
            { #_NAME "FromString", Python_##_NAME##FromString, METH_VARARGS, "Enum From String" }, \
            { #_NAME "ToString", Python_##_NAME##ToString, METH_VARARGS, "Enum To String" },
         // clang-format off
		#include "external/df_serialize/_fillunsetdefines.h"
        #include "Schemas/Schemas.h"
        // clang-format on

        {nullptr, nullptr, 0, nullptr}
    };

    static PyModuleDef pythonModule =
    {
        PyModuleDef_HEAD_INIT, "Host", NULL, -1, pythonModuleMethods,
        NULL, NULL, NULL, NULL
    };

    PyImport_AppendInittab("Host",
        []()
        {
            PyObject* module = PyModule_Create(&pythonModule);
            AddEnums(module);
            return module;
        }
    );

    PyInit_GigiArray();

    // Build a standard python config
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    // Set isolated mode to 1. This prevents python from loading any envionment variables,
    // which isolates this session from any global python the user may have installed.
	config.isolated = 1;

    // Set the `home` field - equivalent to PYTHONHOME environment variable -
    // to our python install directory which we expect to be always relative to the exe dir.
    // This is important because it enables python to find extra modules installed in that location.
    // Another candidate is the working directory, but that may be different if running GigiViewer from another directory
    // so this is a safer option.
    TCHAR exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    // Get the last slash address and fill all space after it with zeros
    TCHAR* pExeName = wcsrchr(exePath, L'\\');
    ZeroMemory(pExeName, (exePath + MAX_PATH - pExeName) * sizeof(TCHAR));
    // Write the python install directory to the end of the exe string
    const int pythonHomeBufSize = 4096;
    wchar_t pythonHomeBuf[pythonHomeBufSize] = { 0 };
    swprintf_s(pythonHomeBuf, L"%s\\GigiViewerDX12\\python\\Python310", exePath);

    config.home = pythonHomeBuf;
    Py_InitializeFromConfig(&config);

    // handle command line parameters
    if (firstPythonArgv >= 0 && firstPythonArgv < argc)
    {
        for (int i = firstPythonArgv; i < argc; ++i)
        {
            auto wtext = Py_DecodeLocale(argv[i], nullptr);
            g_argvText.push_back(wtext);
            PyMem_RawFree(wtext);
        }

        g_argv.resize(g_argvText.size());
        for (size_t i = 0; i < g_argv.size(); ++i)
            g_argv[i] = (wchar_t*)g_argvText[i].c_str();

        PySys_SetArgv(int(g_argv.size()), g_argv.data());
    }
}

void PythonShutdown()
{
	Py_Finalize();
}

bool PythonExecute(const char* fileName)
{
    g_interface->m_scriptLocation = std::filesystem::weakly_canonical(fileName).string();

    // Read the file in
    std::vector<char> fileData;
    {
        FILE* file = nullptr;
        fopen_s(&file, fileName, "rb");
        if (!file)
            return false;

        fseek(file, 0, SEEK_END);
        fileData.resize(ftell(file) + 1);
        fseek(file, 0, SEEK_SET);

        fread(fileData.data(), 1, fileData.size() - 1, file);
        fileData[fileData.size() - 1] = 0;
        fclose(file);
    }

    // execute the script
    PyObject* global = PyDict_New();
    PyObject* ret = PyRun_String(fileData.data(), Py_file_input, global, global);

    g_interface->OnExecuteFinished();

    // report errors if there were any
    if (PyErr_Occurred() != NULL) {
        PyObject* pyExcType;
        PyObject* pyExcValue;
        PyObject* pyExcTraceback;
        PyErr_Fetch(&pyExcType, &pyExcValue, &pyExcTraceback);
        PyErr_NormalizeException(&pyExcType, &pyExcValue, &pyExcTraceback);

        PyObject* str_exc_type = PyObject_Repr(pyExcType);
        PyObject* pyStr = PyUnicode_AsEncodedString(str_exc_type, "utf-8", "Error ~");
        const char* strExcType = PyBytes_AS_STRING(pyStr);

        PyObject* str_exc_value = PyObject_Repr(pyExcValue);
        PyObject* pyExcValueStr = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "Error ~");
        const char* strExcValue = PyBytes_AS_STRING(pyExcValueStr);

        const char* actual_file_name = "";
        if (PyObject_HasAttrString(pyExcValue, "filename"))
        {
            PyObject* file_name = PyObject_GetAttrString(pyExcValue, "filename");
            PyObject* file_name_str = PyObject_Str(file_name);
            PyObject* file_name_unicode = PyUnicode_AsEncodedString(file_name_str, "utf-8", "Error");
            actual_file_name = PyBytes_AsString(file_name_unicode);
        }

        const char* actual_line_no = "";
        if (PyObject_HasAttrString(pyExcValue, "lineno"))
        {
            PyObject* line_no = PyObject_GetAttrString(pyExcValue, "lineno");
            PyObject* line_no_str = PyObject_Str(line_no);
            PyObject* line_no_unicode = PyUnicode_AsEncodedString(line_no_str, "utf-8", "Error");
            actual_line_no = PyBytes_AsString(line_no_unicode);
        }

        g_interface->Log(LogLevel::Error, "Python Error: [%s:%s] %s", actual_file_name, actual_line_no, strExcValue);

        Py_XDECREF(pyExcType);
        Py_XDECREF(pyExcValue);
        Py_XDECREF(pyExcTraceback);

        Py_XDECREF(str_exc_type);
        Py_XDECREF(pyStr);

        Py_XDECREF(str_exc_value);
        Py_XDECREF(pyExcValueStr);

        return false;
    }

    return true;
}