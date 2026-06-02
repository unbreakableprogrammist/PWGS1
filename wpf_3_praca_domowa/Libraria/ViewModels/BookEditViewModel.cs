using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using Libraria.Models;
using Microsoft.Win32;

namespace Libraria.ViewModels
{
    public class BookEditViewModel : BaseViewModel
    {
        private readonly Book _book;
        private readonly bool _isNewBook;
        private string _title = string.Empty;
        private string _author = string.Empty;
        private string _description = string.Empty;
        private string _genre = string.Empty;
        private string _newTag = string.Empty;
        private int _rating;
        private BookStatus _status = BookStatus.Unread;
        private byte[] _coverImage = Array.Empty<byte>();
        private string _coverImagePath = string.Empty;
        private string _filePath = string.Empty;
        private int _pageCount;

        public BookEditViewModel(Book book, bool isNewBook = false)
        {
            _book = book;
            _isNewBook = isNewBook;

            Title = book.Title;
            Author = book.Author;
            Description = book.Description;
            Genre = book.Genre;
            Tags = new ObservableCollection<string>(book.Tags);
            Rating = book.Rating;
            Status = book.Status;
            CoverImage = book.CoverImage;
            CoverImagePath = book.CoverImagePath;
            FilePath = book.FilePath;
            PageCount = book.PageCount;

            StatusOptions = Enum.GetValues<BookStatus>();

            BrowseCoverCommand = new RelayCommand(BrowseCover);
            BrowseBookCommand = new RelayCommand(BrowseBook);
            AddTagCommand = new RelayCommand(AddTag, CanAddTag);
            RemoveTagCommand = new RelayCommand(RemoveTag);
            SaveCommand = new RelayCommand(Save, CanSave);
            CancelCommand = new RelayCommand(Cancel);
        }

        public event EventHandler<CloseRequestedEventArgs>? CloseRequested;

        public string WindowTitle => _isNewBook ? "Add Book" : "Edit Book";

        public IReadOnlyList<BookStatus> StatusOptions { get; }

        public ObservableCollection<string> Tags { get; }

        public string Title
        {
            get => _title;
            set
            {
                if (SetProperty(ref _title, value ?? string.Empty))
                {
                    SaveCommand?.RaiseCanExecuteChanged();
                }
            }
        }

        public string Author
        {
            get => _author;
            set => SetProperty(ref _author, value ?? string.Empty);
        }

        public string Description
        {
            get => _description;
            set => SetProperty(ref _description, value ?? string.Empty);
        }

        public string Genre
        {
            get => _genre;
            set => SetProperty(ref _genre, value ?? string.Empty);
        }

        public string NewTag
        {
            get => _newTag;
            set
            {
                if (SetProperty(ref _newTag, value ?? string.Empty))
                {
                    AddTagCommand?.RaiseCanExecuteChanged();
                }
            }
        }

        public int Rating
        {
            get => _rating;
            set => SetProperty(ref _rating, Math.Clamp(value, 0, 5));
        }

        public BookStatus Status
        {
            get => _status;
            set => SetProperty(ref _status, value);
        }

        public byte[] CoverImage
        {
            get => _coverImage;
            set
            {
                if (SetProperty(ref _coverImage, value ?? Array.Empty<byte>()))
                {
                    OnPropertyChanged(nameof(HasCoverImage));
                }
            }
        }

        public bool HasCoverImage => CoverImage.Length > 0;

        public string CoverImagePath
        {
            get => _coverImagePath;
            set => SetProperty(ref _coverImagePath, value ?? string.Empty);
        }

        public string FilePath
        {
            get => _filePath;
            set
            {
                if (SetProperty(ref _filePath, value ?? string.Empty))
                {
                    OnPropertyChanged(nameof(HasBookFile));
                }
            }
        }

        public bool HasBookFile => !string.IsNullOrWhiteSpace(FilePath);

        public int PageCount
        {
            get => _pageCount;
            set => SetProperty(ref _pageCount, Math.Max(0, value));
        }

        public RelayCommand BrowseCoverCommand { get; }

        public RelayCommand BrowseBookCommand { get; }

        public RelayCommand AddTagCommand { get; }

        public RelayCommand RemoveTagCommand { get; }

        public RelayCommand SaveCommand { get; }

        public RelayCommand CancelCommand { get; }

        private void BrowseCover()
        {
            var dialog = new OpenFileDialog
            {
                Title = "Choose Cover Image",
                Filter = "Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|All files (*.*)|*.*",
                CheckFileExists = true
            };

            if (dialog.ShowDialog() != true)
            {
                return;
            }

            CoverImage = File.ReadAllBytes(dialog.FileName);
            CoverImagePath = dialog.FileName;
        }

        private void BrowseBook()
        {
            var dialog = new OpenFileDialog
            {
                Title = "Choose Book File",
                Filter = "Supported book files (*.md;*.epub;*.pdf)|*.md;*.epub;*.pdf|Markdown files (*.md)|*.md|EPUB files (*.epub)|*.epub|PDF files (*.pdf)|*.pdf|All files (*.*)|*.*",
                CheckFileExists = true
            };

            if (dialog.ShowDialog() == true)
            {
                FilePath = dialog.FileName;
            }
        }

        private bool CanAddTag()
        {
            return !string.IsNullOrWhiteSpace(NewTag)
                && !Tags.Contains(NewTag.Trim(), StringComparer.CurrentCultureIgnoreCase);
        }

        private void AddTag()
        {
            if (!CanAddTag())
            {
                return;
            }

            Tags.Add(NewTag.Trim());
            NewTag = string.Empty;
        }

        private void RemoveTag(object? parameter)
        {
            if (parameter is string tag)
            {
                Tags.Remove(tag);
                AddTagCommand.RaiseCanExecuteChanged();
            }
        }

        private bool CanSave()
        {
            return !string.IsNullOrWhiteSpace(Title);
        }

        private void Save()
        {
            if (!CanSave())
            {
                return;
            }

            _book.Title = Title.Trim();
            _book.Author = Author.Trim();
            _book.Description = Description.Trim();
            _book.Genre = Genre.Trim();
            _book.Tags = new ObservableCollection<string>(Tags);
            _book.Rating = Rating;
            _book.Status = Status;
            _book.CoverImage = CoverImage;
            _book.CoverImagePath = CoverImagePath;
            _book.FilePath = FilePath;
            _book.PageCount = PageCount;
            _book.CurrentPage = Math.Max(1, _book.CurrentPage);
            _book.UpdatedAt = DateTime.Now;

            CloseRequested?.Invoke(this, new CloseRequestedEventArgs(true));
        }

        private void Cancel()
        {
            CloseRequested?.Invoke(this, new CloseRequestedEventArgs(false));
        }
    }

    public sealed class CloseRequestedEventArgs : EventArgs
    {
        public CloseRequestedEventArgs(bool? dialogResult)
        {
            DialogResult = dialogResult;
        }

        public bool? DialogResult { get; }
    }
}
