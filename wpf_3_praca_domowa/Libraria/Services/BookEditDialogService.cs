using System.Linq;
using System.Windows;
using Libraria.ViewModels;
using Libraria.Views;

namespace Libraria.Services
{
    public interface IBookEditDialogService
    {
        bool ShowDialog(BookEditViewModel viewModel);
    }

    public class BookEditDialogService : IBookEditDialogService
    {
        public bool ShowDialog(BookEditViewModel viewModel)
        {
            var window = new EditBookWindow(viewModel)
            {
                Owner = Application.Current.Windows
                    .OfType<Window>()
                    .FirstOrDefault(window => window.IsActive)
                    ?? Application.Current.MainWindow
            };

            void OnCloseRequested(object? sender, CloseRequestedEventArgs e)
            {
                if (e.DialogResult.HasValue)
                {
                    window.DialogResult = e.DialogResult;
                    return;
                }

                window.Close();
            }

            viewModel.CloseRequested += OnCloseRequested;

            try
            {
                return window.ShowDialog() == true;
            }
            finally
            {
                viewModel.CloseRequested -= OnCloseRequested;
            }
        }
    }
}
