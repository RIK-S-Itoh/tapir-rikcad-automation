#include "LibraryCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "Name.hpp"
#include "HashSet.hpp"

AddFilesToEmbeddedLibraryCommand::AddFilesToEmbeddedLibraryCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String AddFilesToEmbeddedLibraryCommand::GetName () const
{
    return "AddFilesToEmbeddedLibrary";
}

GS::Optional<GS::UniString> AddFilesToEmbeddedLibraryCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "files": {
                "$ref": "#/LibraryFileAdditions"
            }
        },
        "additionalProperties": false,
        "required": [
            "files"
        ]
    })";
}

GS::Optional<GS::UniString> AddFilesToEmbeddedLibraryCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState AddFilesToEmbeddedLibraryCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
	auto folderId = API_SpecFolderID::API_EmbeddedProjectLibraryFolderID;

	IO::Location embeddedLibraryFolder;

	if (ACAPI_ProjectSettings_GetSpecFolder (&folderId, &embeddedLibraryFolder) != NoError || IO::Folder (embeddedLibraryFolder).GetStatus () != NoError) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to get embedded library folder.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    GS::Array<GS::ObjectState> files;
    parameters.Get ("files", files);

    for (const GS::ObjectState& file : files) {
        GS::UniString inputPath;
        GS::UniString outputPath;
        if (!file.Get ("inputPath", inputPath) || !file.Get ("outputPath", outputPath) || inputPath.IsEmpty () || outputPath.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing inputPath or outputPath parameters."));
            continue;
        }

        IO::File inputFile (IO::Location (inputPath), IO::File::OnNotFound::Fail);
        if (inputFile.GetStatus () != NoError) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to read file on the given inputPath."));
            continue;
        }

		IO::Location outputFileLoc = embeddedLibraryFolder;
		outputFileLoc.AppendToLocal (IO::RelativeLocation (outputPath));
        IO::Location outputFolder = outputFileLoc;
        if (outputFolder.DeleteLastLocalName () != NoError ||
            IO::fileSystem.CreateFolderTree (outputFolder) != NoError ||
            IO::fileSystem.Copy (inputFile.GetLocation (), outputFileLoc) != NoError) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The given outputPath is not a valid relative path."));
            continue;
        }

        API_LibPart libPart = {};

        libPart.typeID = APILib_PictID;

        GS::UniString typeStr;
        if (file.Get ("type", typeStr)) {
            if (typeStr == "Window") {
                libPart.typeID = APILib_WindowID;
            } else if (typeStr == "Door") {
                libPart.typeID = APILib_DoorID;
            } else if (typeStr == "Object") {
                libPart.typeID = APILib_ObjectID;
            } else if (typeStr == "Lamp") {
                libPart.typeID = APILib_LampID;
            } else if (typeStr == "Room") {
                libPart.typeID = APILib_RoomID;
            } else if (typeStr == "Property") {
                libPart.typeID = APILib_PropertyID;
            } else if (typeStr == "PlanSign") {
                libPart.typeID = APILib_PlanSignID;
            } else if (typeStr == "Label") {
                libPart.typeID = APILib_LabelID;
            } else if (typeStr == "Macro") {
                libPart.typeID = APILib_MacroID;
            } else if (typeStr == "Pict") {
                libPart.typeID = APILib_PictID;
            } else if (typeStr == "ListScheme") {
                libPart.typeID = APILib_ListSchemeID;
            } else if (typeStr == "Skylight") {
                libPart.typeID = APILib_SkylightID;
            } else if (typeStr == "OpeningSymbol") {
                libPart.typeID = APILib_OpeningSymbolID;
            } else {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Unknown library part type."));
                continue;
            }
        }

        libPart.location = &outputFileLoc;

        GSErrCode err = ACAPI_LibraryPart_Register (&libPart);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to add the file to the library."));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
}

GS::Optional<GS::UniString> GetLibrariesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "A list of project libraries.",
                "items": {
                    "type": "object",
                    "description": "Library",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Library name."
                        },
                        "path": {
                            "type": "string",
                            "description": "A filesystem path to library location."
                        },
                        "type": {
                            "type": "string",
                            "description": "Library type."
                        },
                        "available": {
                            "type": "boolean",
                            "description": "Is library not missing."
                        },
                        "readOnly": {
                            "type": "boolean",
                            "description": "Is library not writable."
                        },
                        "twServerUrl": {
                            "type": "string",
                            "description": "URL address of the TeamWork server hosting the library."
                        },
                        "urlWebLibrary": {
                            "type": "string",
                            "description": "URL of the downloaded Internet library."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "type",
                        "path"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraries"
        ]
    })";
}

GetLibrariesCommand::GetLibrariesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetLibrariesCommand::GetName () const
{
    return "GetLibraries";
}

GS::ObjectState GetLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_LibraryInfo> libs;

    GSErrCode err = ACAPI_LibraryManagement_GetLibraries (&libs);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive libraries.");
    }

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("libraries");

    for (const API_LibraryInfo& lib : libs) {
        GS::ObjectState libraryData;
        GS::UniString   type;
        GS::UniString   twServerUrl;
        GS::UniString   urlWebLibrary;
        switch (lib.libraryType) {
            case API_LibraryTypeID::API_Undefined:
                type = "Undefined";
                break;
            case API_LibraryTypeID::API_LocalLibrary:
                type = "LocalLibrary";
                break;
            case API_LibraryTypeID::API_UrlLibrary:
                type = "UrlLibrary";
                urlWebLibrary = lib.twServerUrl;
                break;
            case API_LibraryTypeID::API_BuiltInLibrary:
                type = "BuiltInLibrary";
                break;
            case API_LibraryTypeID::API_EmbeddedLibrary:
                type = "EmbeddedLibrary";
                break;
            case API_LibraryTypeID::API_OtherObject:
                type = "OtherObject";
                break;
            case API_LibraryTypeID::API_UrlOtherObject:
                type = "UrlOtherObject";
                urlWebLibrary = lib.twServerUrl;
                break;
            case API_LibraryTypeID::API_ServerLibrary:
                type = "ServerLibrary";
                twServerUrl = lib.twServerUrl;
                break;
        }

        libraryData.Add ("name", lib.name);
        libraryData.Add ("path", lib.location.ToDisplayText ());
        libraryData.Add ("type", type);
        libraryData.Add ("available", lib.available);
        libraryData.Add ("readOnly", lib.readOnly);
        libraryData.Add ("twServerUrl", twServerUrl);
        libraryData.Add ("urlWebLibrary", urlWebLibrary);
        listAdder (libraryData);
    }

    return response;
}

// NFKC normalization: half-width katakana → full-width, full-width ASCII → half-width, etc.
static GS::UniString NormalizeKana (const GS::UniString& str)
{
    if (str.IsEmpty ()) return str;

    GS::UniChar::Layout* srcBuf = str.CopyUStr ();
    USize srcLen = str.GetLength ();

    GS::UniChar::Layout* nfkcBuf = nullptr;
    USize nfkcLen = 0;
    GS::UniChar::ConvertToCompatibilityComposed (srcBuf, srcLen, &nfkcBuf, &nfkcLen);
    delete[] srcBuf;

    if (nfkcBuf == nullptr) return str;

    GS::UniString result (nfkcBuf, nfkcLen);
    delete[] nfkcBuf;

    return result;
}

static GS::HashSet<GS::UniString> CollectMatchingFileNamesFromXmlCatalogs (
    const IO::Location&   libraryLoc,
    const GS::UniString&  lowerNameFilter)
{
    GS::HashSet<GS::UniString> result;

    IO::Folder folder (libraryLoc);
    if (folder.GetStatus () != NoError) {
        return result;
    }

    folder.Enumerate ([&] (const IO::Name& entryName, bool isFolder) {
        if (isFolder) return;
        if (entryName.GetExtension ().ToLowerCase () != "xml") return;

        IO::Location xmlLoc = libraryLoc;
        xmlLoc.AppendToLocal (IO::RelativeLocation (entryName.ToString ()));

        IO::File xmlFile (xmlLoc, IO::File::OnNotFound::Fail);
        if (xmlFile.GetStatus () != NoError) return;

        USize fileSize = 0;
        if (xmlFile.GetDataLength (&fileSize) != NoError || fileSize == 0) return;

        xmlFile.Open (IO::File::OpenMode::ReadMode);
        char* buffer = new char[fileSize + 1];
        USize bytesRead = 0;
        xmlFile.ReadBin (buffer, fileSize, &bytesRead);
        xmlFile.Close ();
        buffer[bytesRead] = '\0';
        GS::UniString content (buffer, CC_UTF8);
        delete[] buffer;

        const GS::UniString fileNameAttr ("FileName=\"");
        const GS::UniString keywordsOpen ("<Keywords>");
        const GS::UniString keywordsClose ("</Keywords>");

        UIndex searchPos = 0;
        while (searchPos < content.GetLength ()) {
            UIndex fnPos = content.FindFirst (fileNameAttr, searchPos);
            if (fnPos == MaxUIndex) break;

            UIndex fnStart = fnPos + fileNameAttr.GetLength ();
            UIndex fnEnd   = content.FindFirst (GS::UniString ("\""), fnStart);
            if (fnEnd == MaxUIndex) break;

            GS::UniString fileName = content (fnStart, fnEnd - fnStart);
            UIndex nextFn = content.FindFirst (fileNameAttr, fnEnd);

            UIndex kwOpen = content.FindFirst (keywordsOpen, fnEnd);
            if (kwOpen != MaxUIndex && (nextFn == MaxUIndex || kwOpen < nextFn)) {
                UIndex kwClose = content.FindFirst (keywordsClose, kwOpen);
                if (kwClose != MaxUIndex) {
                    UIndex kwStart = kwOpen + keywordsOpen.GetLength ();
                    GS::UniString keywords = content (kwStart, kwClose - kwStart);
                    if (NormalizeKana (keywords).ToLowerCase ().Contains (lowerNameFilter)) {
                        result.Add (NormalizeKana (fileName).ToLowerCase ());
                    }
                }
            }

            searchPos = (nextFn != MaxUIndex) ? nextFn : content.GetLength ();
        }
    });

    return result;
}

static GS::UniString LibTypeToString (API_LibTypeID typeID)
{
    switch (typeID) {
        case APILib_WindowID:        return "Window";
        case APILib_DoorID:          return "Door";
        case APILib_ObjectID:        return "Object";
        case APILib_LampID:          return "Lamp";
        case APILib_RoomID:          return "Room";
        case APILib_MacroID:         return "Macro";
        case APILib_PictID:          return "Pict";
        case APILib_PropertyID:      return "Property";
        case APILib_PlanSignID:      return "PlanSign";
        case APILib_LabelID:         return "Label";
        case APILib_ListSchemeID:    return "ListScheme";
        case APILib_SkylightID:      return "Skylight";
        case APILib_OpeningSymbolID: return "OpeningSymbol";
        default:                     return "Unknown";
    }
}

static bool StringToLibType (const GS::UniString& typeStr, API_LibTypeID& outTypeID)
{
    if (typeStr == "Window")          { outTypeID = APILib_WindowID;        return true; }
    if (typeStr == "Door")            { outTypeID = APILib_DoorID;          return true; }
    if (typeStr == "Object")          { outTypeID = APILib_ObjectID;        return true; }
    if (typeStr == "Lamp")            { outTypeID = APILib_LampID;          return true; }
    if (typeStr == "Room")            { outTypeID = APILib_RoomID;          return true; }
    if (typeStr == "Macro")           { outTypeID = APILib_MacroID;         return true; }
    if (typeStr == "Pict")            { outTypeID = APILib_PictID;          return true; }
    if (typeStr == "Property")        { outTypeID = APILib_PropertyID;      return true; }
    if (typeStr == "PlanSign")        { outTypeID = APILib_PlanSignID;      return true; }
    if (typeStr == "Label")           { outTypeID = APILib_LabelID;         return true; }
    if (typeStr == "ListScheme")      { outTypeID = APILib_ListSchemeID;    return true; }
    if (typeStr == "Skylight")        { outTypeID = APILib_SkylightID;      return true; }
    if (typeStr == "OpeningSymbol")   { outTypeID = APILib_OpeningSymbolID; return true; }
    return false;
}

GetLibraryPartsCommand::GetLibraryPartsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetLibraryPartsCommand::GetName () const
{
    return "GetLibraryParts";
}

GS::Optional<GS::UniString> GetLibraryPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "type": {
                "type": "string",
                "description": "Optional library part type filter. One of: Object, Window, Door, Lamp, Room, Label, Macro, Pict, Property, PlanSign, ListScheme, Skylight, OpeningSymbol.",
                "enum": ["Object", "Window", "Door", "Lamp", "Room", "Label", "Macro", "Pict", "Property", "PlanSign", "ListScheme", "Skylight", "OpeningSymbol"]
            },
            "nameFilter": {
                "type": "string",
                "description": "Optional case-insensitive substring filter for the library part name."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetLibraryPartsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "List of library parts from loaded libraries.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Display name of the library part."
                        },
                        "type": {
                            "type": "string",
                            "description": "Library part type."
                        },
                        "index": {
                            "type": "integer",
                            "description": "Database index of the library part."
                        },
                        "ownUnID": {
                            "type": "string",
                            "description": "Own unique ID of the library part."
                        },
                        "parentUnID": {
                            "type": "string",
                            "description": "Parent unique ID of the library part."
                        },
                        "filePath": {
                            "type": "string",
                            "description": "File system path of the library part."
                        },
                        "missing": {
                            "type": "boolean",
                            "description": "True if the library part definition is missing."
                        },
                        "isPlaceable": {
                            "type": "boolean",
                            "description": "True if the library part can be placed on the plan."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["name", "type", "index", "ownUnID", "missing", "isPlaceable"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["libraryParts"]
    })";
}

GS::ObjectState GetLibraryPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString typeStr;
    bool hasTypeFilter = parameters.Get ("type", typeStr);

    API_LibTypeID filterTypeID = API_ZombieLibID;
    if (hasTypeFilter && !StringToLibType (typeStr, filterTypeID)) {
        return CreateErrorResponse (APIERR_BADPARS,
            GS::UniString::Printf ("Invalid library part type '%T'.", typeStr.ToPrintf ()));
    }

    GS::UniString nameFilter;
    bool hasNameFilter = parameters.Get ("nameFilter", nameFilter);
    GS::UniString lowerNameFilter;
    GS::HashSet<GS::UniString> xmlMatchingFileNames;
    if (hasNameFilter) {
        lowerNameFilter = NormalizeKana (nameFilter).ToLowerCase ();

        GS::Array<API_LibraryInfo> libs;
        ACAPI_LibraryManagement_GetLibraries (&libs);
        for (const API_LibraryInfo& lib : libs) {
            GS::HashSet<GS::UniString> found = CollectMatchingFileNamesFromXmlCatalogs (lib.location, lowerNameFilter);
            for (const GS::UniString& fn : found) {
                xmlMatchingFileNames.Add (fn);
            }
        }
    }

    Int32 count = 0;
    GSErrCode err = ACAPI_LibraryPart_GetNum (&count);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get library part count.");
    }

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("libraryParts");

    for (Int32 i = 1; i <= count; ++i) {
        API_LibPart lp = {};
        lp.index = i;

        if (ACAPI_LibraryPart_Get (&lp) != NoError) {
            if (lp.location != nullptr) {
                delete lp.location;
            }
            continue;
        }

        if (hasTypeFilter && lp.typeID != filterTypeID) {
            if (lp.location != nullptr) {
                delete lp.location;
            }
            continue;
        }

        if (hasNameFilter) {
            bool matches = NormalizeKana (GS::UniString (lp.docu_UName)).ToLowerCase ().Contains (lowerNameFilter);

            if (!matches && lp.location != nullptr) {
                IO::Name fileName;
                if (lp.location->GetLastLocalName (&fileName) == NoError) {
                    GS::UniString lowerFileName = NormalizeKana (fileName.ToString ()).ToLowerCase ();
                    if (lowerFileName.Contains (lowerNameFilter) || xmlMatchingFileNames.Contains (lowerFileName)) {
                        matches = true;
                    }
                }
            }

            if (!matches) {
                if (lp.location != nullptr) {
                    delete lp.location;
                }
                continue;
            }
        }

        GS::ObjectState part;
        part.Add ("name", GS::UniString (lp.docu_UName));
        part.Add ("type", LibTypeToString (lp.typeID));
        part.Add ("index", lp.index);
        part.Add ("ownUnID", GS::UniString (lp.ownUnID));
        part.Add ("parentUnID", GS::UniString (lp.parentUnID));
        part.Add ("missing", lp.missingDef);
        part.Add ("isPlaceable", lp.isPlaceable);

        if (lp.location != nullptr) {
            part.Add ("filePath", lp.location->ToDisplayText ());
            delete lp.location;
        }

        listAdder (part);
    }

    return response;
}

ReloadLibrariesCommand::ReloadLibrariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ReloadLibrariesCommand::GetName () const
{
    return "ReloadLibraries";
}

GS::Optional<GS::UniString> ReloadLibrariesCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ReloadLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_ProjectOperation_ReloadLibraries ();
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to reload libraries.");
    }

    return CreateSuccessfulExecutionResult ();
}
