#include <filesystem>
#include <iostream>

int main()
{
    std::cout << "------------------------------------\n";
    std::cout << "--Surge Script API Generation Tool--\n";
    std::cout << "------------------------------------\n";
    std::cout << std::endl;

    std::string surgeRepoLocation;
    std::string outputLocation;

    std::cout << "Surge Repository location :>>";
    std::cin >> surgeRepoLocation;
    std::cout << std::endl;
    std::cout << "Output location           :>>";
    std::cin >> outputLocation;

    std::filesystem::path outputPath = (outputLocation);
    std::filesystem::create_directory(outputPath);

    std::filesystem::path sourcePath = std::filesystem::path(surgeRepoLocation) / "Engine";
    for (auto& dir : std::filesystem::recursive_directory_iterator(sourcePath))
    {
        auto currentOutputPath = outputPath / std::filesystem::relative(dir.path(), sourcePath);
        auto currnetExtension = currentOutputPath.extension();

        if (std::filesystem::is_directory(dir.path())) // Folder
        {
            std::filesystem::create_directory(currentOutputPath);
        }
        if (currnetExtension == ".hpp" || currnetExtension == ".h" || currnetExtension == ".inl") // Skip source files
        {
            std::filesystem::copy(dir.path(), currentOutputPath, std::filesystem::copy_options::overwrite_existing);
        }
    }

    std::cout << std::endl;
    std::cout << "SurgeAPI Generation Complete!" << std::endl;
    std::cin.get();
    std::cin.get();
}