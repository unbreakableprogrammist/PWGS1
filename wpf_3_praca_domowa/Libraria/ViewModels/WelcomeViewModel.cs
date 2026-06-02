using System;
using Libraria.Services;
using Microsoft.Win32;

namespace Libraria.ViewModels
{
    public class WelcomeViewModel : BaseViewModel
    {
        private readonly RepositoryService _repositoryService;
        private readonly Action<string> _repositoryReady;

        public WelcomeViewModel(RepositoryService repositoryService, Action<string> repositoryReady)
        {
            _repositoryService = repositoryService;
            _repositoryReady = repositoryReady;

            CreateRepositoryCommand = new RelayCommand(CreateRepository);
            OpenRepositoryCommand = new RelayCommand(OpenRepository);
        }

        public RelayCommand CreateRepositoryCommand { get; }

        public RelayCommand OpenRepositoryCommand { get; }

        private void CreateRepository()
        {
            var dialog = new SaveFileDialog
            {
                Title = "Create Repository",
                DefaultExt = ".librepo",
                Filter = "Libraria repository (*.librepo)|*.librepo|JSON files (*.json)|*.json|All files (*.*)|*.*"
            };

            if (dialog.ShowDialog() != true)
            {
                return;
            }

            _repositoryService.Create(dialog.FileName);
            _repositoryReady("Repository created.");
        }

        private void OpenRepository()
        {
            var dialog = new OpenFileDialog
            {
                Title = "Open Repository",
                DefaultExt = ".librepo",
                Filter = "Libraria repository (*.librepo)|*.librepo|JSON files (*.json)|*.json|All files (*.*)|*.*",
                CheckFileExists = true
            };

            if (dialog.ShowDialog() != true)
            {
                return;
            }

            _repositoryService.Open(dialog.FileName);
            _repositoryReady("Repository opened.");
        }
    }
}
