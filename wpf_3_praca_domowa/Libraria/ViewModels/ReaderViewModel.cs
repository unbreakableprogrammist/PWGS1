using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Libraria.Models;
using Libraria.Services;

namespace Libraria.ViewModels
{
    public class ReaderViewModel : BaseViewModel
    {
        private readonly Book _book;
        private readonly Action _goBack;
        private readonly List<string> _pages;
        private readonly string? _baseDirectory;
        private int _currentPage;
        private FlowDocument _currentDocument = new();

        public ReaderViewModel(Book book, int startPage, Action goBack)
        {
            _book = book;
            _goBack = goBack;
            _pages = new MarkdownBookContentService().LoadPages(book.FilePath).ToList();
            _baseDirectory = Path.GetDirectoryName(book.FilePath);

            PreviousCommand = new RelayCommand(PreviousPage, () => CanGoPrevious);
            NextCommand = new RelayCommand(NextPage, () => CanGoNext);
            BackCommand = new RelayCommand(_goBack);

            if (_pages.Count > 0 && _book.PageCount != _pages.Count)
            {
                _book.PageCount = _pages.Count;
            }

            SetCurrentPage(Math.Clamp(startPage, 1, Math.Max(1, _pages.Count)));
        }

        public string Title => _book.Title;

        public string Author => _book.Author;

        public int CurrentPage
        {
            get => _currentPage;
            private set
            {
                if (SetProperty(ref _currentPage, value))
                {
                    _book.CurrentPage = value;
                    OnPropertyChanged(nameof(LocationText));
                    OnPropertyChanged(nameof(CanGoPrevious));
                    OnPropertyChanged(nameof(CanGoNext));
                    PreviousCommand.RaiseCanExecuteChanged();
                    NextCommand.RaiseCanExecuteChanged();
                }
            }
        }

        public int PageCount => _pages.Count;

        public string LocationText => PageCount == 0
            ? "No content"
            : $"Chapter {CurrentPage} of {PageCount}";

        public bool CanGoPrevious => CurrentPage > 1;

        public bool CanGoNext => CurrentPage < PageCount;

        public FlowDocument CurrentDocument
        {
            get => _currentDocument;
            private set => SetProperty(ref _currentDocument, value);
        }

        public RelayCommand PreviousCommand { get; }

        public RelayCommand NextCommand { get; }

        public RelayCommand BackCommand { get; }

        private void PreviousPage()
        {
            if (CanGoPrevious)
            {
                SetCurrentPage(CurrentPage - 1);
            }
        }

        private void NextPage()
        {
            if (CanGoNext)
            {
                SetCurrentPage(CurrentPage + 1);
            }
        }

        private void SetCurrentPage(int page)
        {
            CurrentPage = page;
            CurrentDocument = BuildDocument(
                _pages.Count == 0 ? "No readable Markdown content." : _pages[CurrentPage - 1],
                _baseDirectory);
        }

        private static FlowDocument BuildDocument(string markdown, string? baseDirectory)
        {
            var document = new FlowDocument
            {
                PagePadding = new Thickness(36),
                FontFamily = new FontFamily("Segoe UI"),
                FontSize = 15,
                LineHeight = 24
            };

            var paragraphLines = new List<string>();

            foreach (var rawLine in markdown.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = rawLine.TrimEnd();

                if (string.IsNullOrWhiteSpace(line))
                {
                    AddParagraph(document, paragraphLines);
                    continue;
                }

                if (TryParseImage(line.Trim(), out var altText, out var imagePath))
                {
                    AddParagraph(document, paragraphLines);
                    AddImage(document, altText, imagePath, baseDirectory);
                    continue;
                }

                if (line.StartsWith("#", StringComparison.Ordinal))
                {
                    AddParagraph(document, paragraphLines);
                    AddHeading(document, line);
                    continue;
                }

                if (line.StartsWith("- ", StringComparison.Ordinal) || line.StartsWith("* ", StringComparison.Ordinal))
                {
                    AddParagraph(document, paragraphLines);
                    AddBullet(document, line[2..]);
                    continue;
                }

                paragraphLines.Add(line);
            }

            AddParagraph(document, paragraphLines);
            return document;
        }

        private static bool TryParseImage(string line, out string altText, out string imagePath)
        {
            altText = string.Empty;
            imagePath = string.Empty;

            if (!line.StartsWith("![", StringComparison.Ordinal))
            {
                return false;
            }

            var altEnd = line.IndexOf("](", StringComparison.Ordinal);
            if (altEnd < 2 || !line.EndsWith(")", StringComparison.Ordinal))
            {
                return false;
            }

            altText = line[2..altEnd].Trim();
            imagePath = line[(altEnd + 2)..^1].Trim().Trim('"');
            return !string.IsNullOrWhiteSpace(imagePath);
        }

        private static void AddImage(FlowDocument document, string altText, string imagePath, string? baseDirectory)
        {
            var resolvedPath = ResolveImagePath(imagePath, baseDirectory);
            if (string.IsNullOrWhiteSpace(resolvedPath) || !File.Exists(resolvedPath))
            {
                var missingText = string.IsNullOrWhiteSpace(altText)
                    ? $"Image not found: {imagePath}"
                    : $"Image not found: {altText}";
                document.Blocks.Add(new Paragraph(new Run(missingText))
                {
                    Foreground = Brushes.Gray,
                    FontStyle = FontStyles.Italic,
                    Margin = new Thickness(0, 0, 0, 12)
                });
                return;
            }

            var bitmap = new BitmapImage();
            using (var stream = File.OpenRead(resolvedPath))
            {
                bitmap.BeginInit();
                bitmap.CacheOption = BitmapCacheOption.OnLoad;
                bitmap.StreamSource = stream;
                bitmap.EndInit();
            }

            bitmap.Freeze();

            var image = new Image
            {
                Source = bitmap,
                Stretch = Stretch.Uniform,
                MaxWidth = 680,
                MaxHeight = 420,
                Margin = new Thickness(0, 4, 0, 10)
            };

            document.Blocks.Add(new BlockUIContainer(image)
            {
                Margin = new Thickness(0, 0, 0, 8)
            });

            if (!string.IsNullOrWhiteSpace(altText))
            {
                document.Blocks.Add(new Paragraph(new Run(altText))
                {
                    FontSize = 12,
                    Foreground = Brushes.Gray,
                    FontStyle = FontStyles.Italic,
                    Margin = new Thickness(0, 0, 0, 12)
                });
            }
        }

        private static string ResolveImagePath(string imagePath, string? baseDirectory)
        {
            if (Uri.TryCreate(imagePath, UriKind.Absolute, out var uri))
            {
                return uri.IsFile ? uri.LocalPath : string.Empty;
            }

            if (Path.IsPathRooted(imagePath))
            {
                return imagePath;
            }

            return string.IsNullOrWhiteSpace(baseDirectory)
                ? imagePath
                : Path.GetFullPath(Path.Combine(baseDirectory, imagePath));
        }

        private static void AddHeading(FlowDocument document, string line)
        {
            var level = line.TakeWhile(character => character == '#').Count();
            var text = line[level..].Trim();
            var paragraph = new Paragraph(new Run(text))
            {
                FontWeight = FontWeights.SemiBold,
                FontSize = level switch
                {
                    1 => 28,
                    2 => 22,
                    _ => 18
                },
                Margin = new Thickness(0, 0, 0, 14)
            };

            document.Blocks.Add(paragraph);
        }

        private static void AddBullet(FlowDocument document, string text)
        {
            var paragraph = new Paragraph
            {
                Margin = new Thickness(18, 0, 0, 8)
            };

            paragraph.Inlines.Add(new Run("• ") { FontWeight = FontWeights.Bold });
            AddInlineMarkdown(paragraph, text);
            document.Blocks.Add(paragraph);
        }

        private static void AddParagraph(FlowDocument document, List<string> lines)
        {
            if (lines.Count == 0)
            {
                return;
            }

            var paragraph = new Paragraph
            {
                Margin = new Thickness(0, 0, 0, 12)
            };

            AddInlineMarkdown(paragraph, string.Join(" ", lines));
            document.Blocks.Add(paragraph);
            lines.Clear();
        }

        private static void AddInlineMarkdown(Paragraph paragraph, string text)
        {
            var parts = text.Split("**");

            for (var index = 0; index < parts.Length; index++)
            {
                var run = new Run(StripInlineMarkdown(parts[index]));

                if (index % 2 == 1)
                {
                    run.FontWeight = FontWeights.Bold;
                }

                paragraph.Inlines.Add(run);
            }
        }

        private static string StripInlineMarkdown(string text)
        {
            return text
                .Replace("**", string.Empty)
                .Replace("__", string.Empty)
                .Replace("*", string.Empty)
                .Replace("_", string.Empty)
                .Replace("`", string.Empty);
        }
    }
}
