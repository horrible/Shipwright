#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "ClientModelLoader.h"
#include "soh/Enhancements/nametag.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/Enhancements/mod_menu.h"

#include <ship/Context.h>
#include <ship/resource/ResourceLoader.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/FolderArchive.h>
#include <ship/resource/archive/O2rArchive.h>
#include <ship/resource/archive/OtrArchive.h>
#include <ship/utils/StrHash64.h>
#include <ship/utils/StringHelper.h>

bool IsValidExtension(std::string extension);

uint64_t orderReferencesRecursively(
    const uint64_t& crchash,
    const uint64_t& crchash2,
    const std::string& filename,
    std::unordered_map<uint64_t, std::string>& unorderedReferences,
    std::vector<std::string>& orderedReferences,
    std::set<std::string>& included,
    const std::shared_ptr<Ship::Archive>& root);

std::vector<std::tuple<std::string, std::string, std::string>> clientMods;

void populateClientModelList(std::filesystem::directory_entry p) {
    clientMods.clear();
    clientMods.push_back(std::make_tuple("Default", "objects/object_link_boy/gLinkAdultSkel", "objects/object_link_child/gLinkChildSkel"));
    // store full archive content under its own name for usage by anchor clients
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(p)) {
        std::string modName;
        std::string anchorPath;
        bool validAnchor = false;
        std::shared_ptr<Ship::Archive> archive = nullptr;

        if ((entry.is_directory() // yay, we have a likely valid archive
             || (entry.is_regular_file() && IsValidExtension(entry.path().extension().generic_string())))) {

            std::string extension = entry.path().extension().generic_string();

            if (entry.is_directory()) {
                archive = std::make_shared<Ship::FolderArchive>(entry.path().generic_string());
                modName = entry.path().filename().generic_string();
                // vvv comment made by some idiot who hadn't yet had to make sure bones were in the right order
                // :eye_roll_emoji: vvv
                // suuuuper fun. okay. unpack the o2r/otr. check if it has a valid path for adult/child, and only
                // include if it does
            } else if (entry.is_regular_file()) {
                if (StringHelper::IEquals(extension, ".o2r")) {
                    archive = std::make_shared<Ship::O2rArchive>(entry.path().generic_string());
                    modName = entry.path().stem().generic_string();
#ifdef INCLUDE_MPQ_SUPPORT
                } else if (StringHelper::IEquals(extension, ".otr")) {
                    archive = std::make_shared<Ship::OtrArchive>(entry.path().generic_string());
                    modName = entry.path().stem().generic_string();
#endif
                }
            }

            if ((archive != nullptr) && archive->Open()) {
                std::shared_ptr<Ship::File> adultPath = archive->LoadFile("alt/objects/object_link_boy/gLinkAdultSkel");
                std::shared_ptr<Ship::File> childPath =
                    archive->LoadFile("alt/objects/object_link_child/gLinkChildSkel");
                if (adultPath != nullptr || childPath != nullptr) {
                    // good enough. probably has a skeleton
                    anchorPath = p.path().generic_string() + '/' + modName;
                    validAnchor = true;
                }
            }
            if (validAnchor) {
                std::string modPrefix = Ship::IResource::gAltAssetPrefix + modName + '/';
                std::vector<std::string> orderedReferences;
                std::unordered_map<uint64_t, std::string> unorderedReferences = *archive->ListFiles();
                unorderedReferences.erase(CRC64(""));
                uint64_t numFiles = unorderedReferences.size();

                clientMods.push_back(std::make_tuple(
                    modName,
                    modPrefix + "objects/object_link_boy/gLinkAdultSkel",
                    modPrefix + "objects/object_link_child/gLinkChildSkel"));

                //ensure all files are added before they're referenced because nobody likes missing bones
                // i genuinely don't know if this is actually needed. i was having lots of problems loading models and it looked like it was missing shit during the load
                // this fixed that, but idk if that was just logger noise and didn't actually matter
                std::set<std::string> included; //because a set is easier to look at than a vector and probably faster

                //while archive has unordered files
                while ((orderedReferences.size() < numFiles) && (unorderedReferences.size() != 0)) {
                    std::set<uint64_t> hash2trash;

                    //for every remaining unordered file
                    for (auto const& [hash1, filename] : unorderedReferences) {

                        //generate two crc hashes for file since some have alt prefix and some don't and i don't want to think of something better
                        //this is mostly to account for how in the recurse flow the child references won't come with a a hash, so i have to decide which to use
                        //and i'm just not going to do that thanks <3
                        uint64_t hash2;
                        if (filename.starts_with(Ship::IResource::gAltAssetPrefix)) {
                            hash2 = CRC64(filename.substr(Ship::IResource::gAltAssetPrefix.size()).c_str());
                        } else {
                            hash2 = CRC64((Ship::IResource::gAltAssetPrefix + filename).c_str());
                        }
                        uint64_t finishedHash = orderReferencesRecursively(hash1, hash2, filename, unorderedReferences, orderedReferences, included, archive);

                        if (finishedHash != UINT64_MAX) {
                            hash2trash.emplace(finishedHash);
                        }
                    }

                    //these apes do not in fact wanna loop forever
                    for (uint64_t hash : hash2trash) {
                        unorderedReferences.erase(hash);
                    }
                }


                //and, finally, add everything from first to last. maybe that's backwards but we'll find out when we exec
                for (std::string filename : orderedReferences) {
                    std::string anchorLoc = modPrefix + (filename.starts_with("alt/") ? filename.substr(4) : filename);
                    std::shared_ptr<Ship::File> data = archive->LoadFile(filename);
                    std::string dataStr;
                    dataStr.assign(data->Buffer->begin(), data->Buffer->end());

                    
                    //probably want to combine this for with the literal exact same one in the recurse but that is a problem for when i am not annoyed by c++
                    size_t end = -1;
                    for (size_t start = dataStr.find("\"objects/"); start != std::string::npos; start = dataStr.find("\"objects/", start + 1)) {
                        end = dataStr.find('"', start + 1);
                        if (included.contains(Ship::IResource::gAltAssetPrefix + dataStr.substr(start + 1, end - start - 1))) {
                            dataStr.insert(start + 1, modPrefix);
                            end += modPrefix.size();
                        }
                    }

                    data->Buffer->assign(dataStr.begin(), dataStr.end());
                    
                    std::shared_ptr<Ship::IResource> resource =
                        Ship::Context::GetRawInstance()->GetResourceManager()->GetResourceLoader()->LoadResource(anchorLoc, data);
                    Ship::Context::GetRawInstance()->GetResourceManager()->CacheExternalResource(anchorLoc , resource);
                }
            }
        }
        if (archive != nullptr) {
            archive->Close();
        }
    }
}


uint64_t orderReferencesRecursively(
    const uint64_t& crchash,
    const uint64_t& crchash2,
    const std::string& filename,
    std::unordered_map<uint64_t, std::string>& unorderedReferences,
    std::vector<std::string>& orderedReferences,
    std::set<std::string>& included,
    const std::shared_ptr<Ship::Archive>& root) {

    //file already checked for references, no need to recheck
    //this shouldn't happen, but i probably fucked up elsewhere so it will
    if (included.contains(filename)) {
        return crchash;
    }

    std::shared_ptr<Ship::File> data = root->LoadFile(filename);
    std::string dataStr;
    uint64_t totalChildren = 0;
    uint64_t finishedChildren = 0;

    //no file, so no references
    //if the file isn't a file we care about, skip and drop
    if (data == nullptr) {
        //seems like the sort of scenario that shouldh't happen but we won't open that can of woims
        return crchash;
    } else {
        dataStr.assign(data->Buffer->begin(), data->Buffer->end());
        if (!dataStr.starts_with('<') && dataStr.find_first_of('<') != std::string::npos) {
            //trim any white space from the start of the string because people hate me
            dataStr.erase(0, dataStr.find_first_not_of(" \t\n\r"));
            if (dataStr.starts_with('<')) {
            } else {
                //this is not an xml file. likely an image or animation or something
                //we should still add it because it's probably needed.
                orderedReferences.insert(orderedReferences.begin(), filename);
                included.insert(filename);
                return crchash;
            }
        }
    }
    //FINALLY now we check if the file has references. yay string math >.>
    size_t end = -1;
    //if file has references, for every reference in file
    for (size_t start = dataStr.find("objects/", end + 1); start != std::string::npos; start = dataStr.find("objects/", end + 1)) {
        end = dataStr.find('"', start + 1);
        std::string child = dataStr.substr(start, end - start);
        totalChildren++;

        //can skip reference if the file for it has already been checked
        //if reference not in placementvector
        if (!included.contains(child) &&  !included.contains(Ship::IResource::gAltAssetPrefix + child)) {
            //generate crc hashes for file
            uint64_t hash1 = CRC64(child.c_str());
            uint64_t hash2 = CRC64((Ship::IResource::gAltAssetPrefix + child).c_str());

            // if you're using models in your model but aren't copying them in... why are you the way that you are?
            if (!unorderedReferences.contains(hash1) && !unorderedReferences.contains(hash2)) {
                //skip adding >:( consider it found
                finishedChildren++;
            } else {
                //otherwise if it's here then recuse to oblivion and back baby
                uint64_t fullyReferencedHash =
                    orderReferencesRecursively(hash1, hash2, Ship::IResource::gAltAssetPrefix + child, unorderedReferences, orderedReferences, included, root);
                if (fullyReferencedHash != UINT64_MAX) {
                    finishedChildren++;
                }
            }
        } else {
            finishedChildren++;
        }
    }

    // append file to to placementvector if it has no children left to be added before it
    if (totalChildren == finishedChildren) {
        orderedReferences.insert(orderedReferences.end(), filename);
        included.insert(filename);
        return crchash;
    }

    return UINT64_MAX;
}
