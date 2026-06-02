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

            return window.ShowDialog() == true;
        }
    }
}
