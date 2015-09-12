//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#include <Atomic/IO/Log.h>
#include <Atomic/IO/File.h>
#include <Atomic/IO/FileSystem.h>
#include <Atomic/Resource/JSONFile.h>

#include "Atomic/Core/ProcessUtils.h"

#include "JSBind.h"
#include "JSBPackage.h"
#include "JSBModule.h"
#include "JSBHeader.h"
#include "JSBClass.h"
#include "JSBEnum.h"
#include "JSBModuleWriter.h"
#include "JSBType.h"

namespace ToolCore
{

JSBModule::JSBModule(Context* context, JSBPackage* package) : Object(context),
    package_(package)
{

}

JSBModule::~JSBModule()
{

}

Vector<SharedPtr<JSBClass>> JSBModule::GetClasses()
{
    return classes_.Values();
}

Vector<SharedPtr<JSBEnum>> JSBModule::GetEnums()
{
    return enums_.Values();
}

void JSBModule::PreprocessHeaders()
{
    for (unsigned i = 0; i < headers_.Size(); i++)
    {
        headers_[i]->VisitPreprocess();
    }
}

void JSBModule::VisitHeaders()
{
    for (unsigned i = 0; i < headers_.Size(); i++)
    {
        headers_[i]->VisitHeader();
    }

    // validate that all classes found
    for (unsigned i = 0; i < classnames_.Size(); i++)
    {
        JSBClass* cls = GetClass(classnames_[i]);

        if (!cls)
        {
            ErrorExit(ToString("Module class not found %s", classnames_[i].CString()));
        }
    }

    ProcessOverloads();
    ProcessExcludes();
    ProcessTypeScriptDecl();
    ProcessHaxeDecl();
}

void JSBModule::PreprocessClasses()
{
    HashMap<StringHash, SharedPtr<JSBClass> >::Iterator itr;

    for (itr = classes_.Begin(); itr != classes_.End(); itr++)
    {
        itr->second_->Preprocess();
    }
}

void JSBModule::ProcessClasses()
{
    HashMap<StringHash, SharedPtr<JSBClass> >::Iterator itr;

    for (itr = classes_.Begin(); itr != classes_.End(); itr++)
    {
        itr->second_->Process();
    }
}

void JSBModule::PostProcessClasses()
{
    HashMap<StringHash, SharedPtr<JSBClass> >::Iterator itr;

    for (itr = classes_.Begin(); itr != classes_.End(); itr++)
    {
        itr->second_->PostProcess();
    }
}

void JSBModule::ProcessOverloads()
{
    // overloads

    JSONValue root = moduleJSON_->GetRoot();

    JSONValue overloads = root.Get("overloads");

    if (overloads.IsObject())
    {
        Vector<String> childNames = overloads.GetObject().Keys();

        for (unsigned j = 0; j < childNames.Size(); j++)
        {
            String classname = childNames.At(j);

            JSBClass* klass = GetClass(classname);

            if (!klass)
            {
                ErrorExit("Bad overload klass");
            }

            JSONValue classoverloads = overloads.Get(classname);

            Vector<String> functionNames = classoverloads.GetObject().Keys();

            for (unsigned k = 0; k < functionNames.Size(); k++)
            {
                JSONValue _sig = classoverloads.Get(functionNames[k]);

                if (!_sig.IsArray())
                {
                    ErrorExit("Bad overload defintion");
                }

                JSONArray sig = _sig.GetArray();

                Vector<String> values;
                for (unsigned x = 0; x < sig.Size(); x++)
                {
                    values.Push(sig[x].GetString());
                }

                JSBFunctionSignature* fo = new JSBFunctionSignature(functionNames[k], values);
                klass->AddFunctionOverride(fo);

            }
        }
    }
}

void JSBModule::ProcessExcludes()
{
    // excludes

    JSONValue root = moduleJSON_->GetRoot();

    JSONValue excludes = root.Get("excludes");

    if (excludes.IsObject())
    {
        Vector<String> childNames = excludes.GetObject().Keys();

        for (unsigned j = 0; j < childNames.Size(); j++)
        {
            String classname = childNames.At(j);

            JSBClass* klass = GetClass(classname);

            if (!klass)
            {
                ErrorExit("Bad exclude klass");
            }

            JSONValue classexcludes = excludes.Get(classname);

            Vector<String> functionNames = classexcludes.GetObject().Keys();

            for (unsigned k = 0; k < functionNames.Size(); k++)
            {
                JSONValue _sig = classexcludes.Get(functionNames[k]);

                if (!_sig.IsArray())
                {
                    ErrorExit("Bad exclude defintion");
                }

                JSONArray sig = _sig.GetArray();

                Vector<String> values;
                for (unsigned x = 0; x < sig.Size(); x++)
                {
                    values.Push(sig[x].GetString());
                }

                JSBFunctionSignature* fe = new JSBFunctionSignature(functionNames[k], values);
                klass->AddFunctionExclude(fe);

            }
        }
    }
}

void JSBModule::ProcessTypeScriptDecl()
{
    // TypeScript declarations

    JSONValue root = moduleJSON_->GetRoot();

    JSONValue decl = root.Get("typescript_decl");

    if (decl.IsObject())
    {
        Vector<String> childNames = decl.GetObject().Keys();

        for (unsigned j = 0; j < childNames.Size(); j++)
        {
            String classname = childNames.At(j);

            JSBClass* klass = GetClass(classname);

            if (!klass)
            {
                ErrorExit("Bad TypeScript decl klass");
            }

            JSONArray classdecl = decl.Get(classname).GetArray();

            for (unsigned k = 0; k < classdecl.Size(); k++)
            {
                klass->AddTypeScriptDecl(classdecl[k].GetString());
            }
        }
    }
}

void JSBModule::ProcessHaxeDecl()
{
    // Haxe declarations

    JSONValue root = moduleJSON_->GetRoot();

    JSONValue decl = root.Get("haxe_decl");

    if (decl.IsObject())
    {
        Vector<String> childNames = decl.GetObject().Keys();

        for (unsigned j = 0; j < childNames.Size(); j++)
        {
            String classname = childNames.At(j);

            JSBClass* klass = GetClass(classname);

            if (!klass)
            {
                ErrorExit("Bad Haxe decl class");
            }

            JSONArray classdecl = decl.Get(classname).GetArray();

            for (unsigned k = 0; k < classdecl.Size(); k++)
            {
                klass->AddHaxeDecl(classdecl[k].GetString());
            }
        }
    }
}

void JSBModule::ScanHeaders()
{
    JSBind* jsbind = GetSubsystem<JSBind>();
    FileSystem* fs = GetSubsystem<FileSystem>();

    const String& sourceRoot = jsbind->GetSourceRootFolder();

    for (unsigned i = 0; i < sourceDirs_.Size(); i++)
    {
        const String& dir = sourceRoot + sourceDirs_[i] + "/";

        Vector<String> fileNames;
        fs->ScanDir(fileNames, dir, "*.h", SCAN_FILES, false);

        for (unsigned k = 0; k < fileNames.Size(); k++)
        {
            String filepath = dir + fileNames[k];

            SharedPtr<JSBHeader> header(new JSBHeader(context_, this, filepath));

            // Parse the C++ header
            header->Parse();

            headers_.Push(header);
        }

    }

}

JSBClass* JSBModule::GetClass(const String& name)
{
    if (classes_.Contains(name))
        return classes_[name];

    return 0;
}

void JSBModule::RegisterClass(String name)
{
    String nativeName = name;

    if (classnames_.Contains(name))
    {
        if (classRenames_.Contains(name))
        {
            name = classRenames_[name];
        }

        if (JSBPackage::GetClassAllPackages(nativeName))
        {
            ErrorExit(ToString("Class collision: %s", name.CString()));
        }

        JSBClass* cls = new JSBClass(context_, this, name, nativeName);

        classes_[nativeName] = cls;

        package_->RegisterClass(cls);
    }
}

void JSBModule::RegisterEnum(JSBEnum* jenum)
{
    if (JSBPackage::GetClassAllPackages(jenum->GetName()))
    {
        ErrorExit(ToString("Enum collision: %s", jenum->GetName().CString()));
    }

    enums_[jenum->GetName()] = jenum;

}

JSBEnum* JSBModule::GetEnum(const String& name)
{
    if (enums_.Contains(name))
    {
        return enums_[name];
    }

    return 0;

}

bool JSBModule::ContainsConstant(const String& constantName)
{
    return constants_.Contains(constantName);
}

void JSBModule::RegisterConstant(const String& constantName, unsigned type)
{
    // MAX_CASCADE_SPLITS is defined differently for desktop/mobile
    if (constantName == "MAX_CASCADE_SPLITS" && JSBPackage::ContainsConstantAllPackages(constantName))
    {
        return;
    }

    if (JSBPackage::ContainsConstantAllPackages(constantName))
    {
        ErrorExit(ToString("Constant collision: %s", constantName.CString()));
    }

    constants_[constantName] = new JSBPrimitiveType(type);
}

bool JSBModule::Load(const String& jsonFilename)
{
    JSBind* jsbind = GetSubsystem<JSBind>();

    LOGINFOF("Loading Module: %s", jsonFilename.CString());

    SharedPtr<File> jsonFile(new File(context_, jsonFilename));

    if (!jsonFile->IsOpen())
    {
        LOGERRORF("Unable to open module json: %s", jsonFilename.CString());
        return false;
    }

    moduleJSON_ = new JSONFile(context_);

    if (!moduleJSON_->BeginLoad(*jsonFile))
    {
        LOGERRORF("Unable to parse module json: %s", jsonFilename.CString());
        return false;
    }

    JSONValue root = moduleJSON_->GetRoot();

    name_ = root.Get("name").GetString();

    JSONValue requires = root.Get("requires");

    if (requires.IsArray())
    {
        for (unsigned j = 0; j < requires.GetArray().Size(); j++)
        {
            requirements_.Push(requires[j].GetString());
        }

    }

    JSONArray classes = root.Get("classes").GetArray();

    for (unsigned i = 0; i < classes.Size(); i++)
    {
        classnames_.Push(classes[i].GetString());
    }

    JSONValue classes_rename = root.Get("classes_rename");

    if (classes_rename.IsObject())
    {
        Vector<String> childNames = classes_rename.GetObject().Keys();
        for (unsigned j = 0; j < childNames.Size(); j++)
        {
            String classname = childNames.At(j);
            String crename = classes_rename.Get(classname).GetString();
            classRenames_[classname] = crename;
        }

    }

    JSONValue includes = root.Get("includes");

    if (includes.IsArray())
    {
        for (unsigned j = 0; j < includes.GetArray().Size(); j++)
        {
            includes_.Push(includes.GetArray()[j].GetString());

        }
    }

    JSONValue sources = root.Get("sources");

    for (unsigned i = 0; i < sources.GetArray().Size(); i++)
    {
        sourceDirs_.Push(sources.GetArray()[i].GetString());
    }

    if (name_ == "Graphics")
    {
#ifdef _MSC_VER
        if (jsbind->GetPlatform() == "ANDROID" || jsbind->GetPlatform() == "WEB")
        {
            sourceDirs_.Push("Source/Atomic/Graphics/OpenGL");
        }
        else
        {
#ifdef ATOMIC_D3D11
            sourceDirs_.Push("Source/Atomic/Graphics/Direct3D11");
#else
            sourceDirs_.Push("Source/Atomic/Graphics/Direct3D9");
#endif
        }
#else
        sourceDirs_.Push("Source/Atomic/Graphics/OpenGL");
#endif
    }


    ScanHeaders();

    return true;
}

void JSBModule::GenerateSource(const String& outPath)
{
    JSBModuleWriter writer(this);
    writer.GenerateSource(source_);

    String filepath = outPath + "/JSModule" + name_ + ".cpp";
    File file(context_);
    file.Open(filepath, FILE_WRITE);
    file.Write(source_.CString(), source_.Length());
    file.Close();
}

}
