;;;

;;; Code:
(eval-when-compile (require 'cl-lib))
(or (featurep 'gtk4)
    (error "%s: Loading gtk4-win.el but not compiled for pure Gtk+-3."
           (invocation-name)))

;; Documentation-purposes only: actually loaded in loadup.el.
(require 'term/common-win)
(require 'frame)
(require 'mouse)
(require 'scroll-bar)
(require 'faces)
(require 'menu-bar)
(require 'fontset)
(require 'dnd)

(defgroup gtk4 nil
  "Pure-GTK specific features."
  :group 'environment)

;;;; Command line argument handling.

(defvar x-invocation-args)
;; Set in term/common-win.el; currently unused by Gtk's x-open-connection.
(defvar x-command-line-resources)

;; gtk4term.c.
(defvar gtk4-input-file)

(defun gtk4-handle-nxopen (_switch &optional temp)
  (setq unread-command-events (append unread-command-events
                                      (if temp '(gtk4-open-temp-file)
                                        '(gtk4-open-file)))
        gtk4-input-file (append gtk4-input-file (list (pop x-invocation-args)))))

(defun gtk4-handle-nxopentemp (switch)
  (gtk4-handle-nxopen switch t))

(defun gtk4-ignore-1-arg (_switch)
  (setq x-invocation-args (cdr x-invocation-args)))

;;;; Keyboard mapping.

(define-obsolete-variable-alias 'gtk4-alternatives-map 'x-alternatives-map "24.1")

(define-key global-map [home] 'beginning-of-buffer)
(define-key global-map [end] 'end-of-buffer)
(define-key global-map [kp-home] 'beginning-of-buffer)
(define-key global-map [kp-end] 'end-of-buffer)
(define-key global-map [kp-prior] 'scroll-down-command)
(define-key global-map [kp-next] 'scroll-up-command)

;;;; File handling.

(defun x-file-dialog (prompt dir default_filename mustmatch only_dir_p)
"Read file name, prompting with PROMPT in directory DIR.
Use a file selection dialog.  Select DEFAULT-FILENAME in the dialog's file
selection box, if specified.  If MUSTMATCH is non-nil, the returned file
or directory must exist.

This function is only defined on GTK4, MS Windows, and X Windows with the
Motif or Gtk toolkits.  With the Motif toolkit, ONLY-DIR-P is ignored.
Otherwise, if ONLY-DIR-P is non-nil, the user can only select directories."
  (gtk4-read-file-name prompt dir mustmatch default_filename only_dir_p))

(defun gtk4-open-file-using-panel ()
  "Pop up open-file panel, and load the result in a buffer."
  (interactive)
  ;; Prompt dir defaultName isLoad initial.
  (setq gtk4-input-file (gtk4-read-file-name "Select File to Load" nil t nil))
  (if gtk4-input-file
      (and (setq gtk4-input-file (list gtk4-input-file)) (gtk4-find-file))))

(defun gtk4-write-file-using-panel ()
  "Pop up save-file panel, and save buffer in resulting name."
  (interactive)
  (let (gtk4-output-file)
    ;; Prompt dir defaultName isLoad initial.
    (setq gtk4-output-file (gtk4-read-file-name "Save As" nil nil nil))
    (message gtk4-output-file)
    (if gtk4-output-file (write-file gtk4-output-file))))

(defcustom gtk4-pop-up-frames 'fresh
  "Non-nil means open files upon request from the Workspace in a new frame.
If t, always do so.  Any other non-nil value means open a new frame
unless the current buffer is a scratch buffer."
  :type '(choice (const :tag "Never" nil)
                 (const :tag "Always" t)
                 (other :tag "Except for scratch buffer" fresh))
  :version "23.1"
  :group 'gtk4)

(declare-function gtk4-hide-emacs "gtk4fns.c" (on))

(defun gtk4-find-file ()
  "Do a `find-file' with the `gtk4-input-file' as argument."
  (interactive)
  (let* ((f (file-truename
	     (expand-file-name (pop gtk4-input-file)
			       command-line-default-directory)))
         (file (find-file-noselect f))
         (bufwin1 (get-buffer-window file 'visible))
         (bufwin2 (get-buffer-window "*scratch*" 'visible)))
    (cond
     (bufwin1
      (select-frame (window-frame bufwin1))
      (raise-frame (window-frame bufwin1))
      (select-window bufwin1))
     ((and (eq gtk4-pop-up-frames 'fresh) bufwin2)
      (gtk4-hide-emacs 'activate)
      (select-frame (window-frame bufwin2))
      (raise-frame (window-frame bufwin2))
      (select-window bufwin2)
      (find-file f))
     (gtk4-pop-up-frames
      (gtk4-hide-emacs 'activate)
      (let ((pop-up-frames t)) (pop-to-buffer file nil)))
     (t
      (gtk4-hide-emacs 'activate)
      (find-file f)))))


(defun gtk4-drag-n-drop (event &optional new-frame force-text)
  "Edit the files listed in the drag-n-drop EVENT.
Switch to a buffer editing the last file dropped."
  (interactive "e")
  (let* ((window (posn-window (event-start event)))
         (arg (car (cdr (cdr event))))
         (type (car arg))
         (data (car (cdr arg)))
         (url-or-string (cond ((eq type 'file)
                               (concat "file:" data))
                              (t data))))
    (set-frame-selected-window nil window)
    (when new-frame
      (select-frame (make-frame)))
    (raise-frame)
    (setq window (selected-window))
    (if force-text
        (dnd-insert-text window 'private data)
      (dnd-handle-one-url window 'private url-or-string))))


(defun gtk4-drag-n-drop-other-frame (event)
  "Edit the files listed in the drag-n-drop EVENT, in other frames.
May create new frames, or reuse existing ones.  The frame editing
the last file dropped is selected."
  (interactive "e")
  (gtk4-drag-n-drop event t))

(defun gtk4-drag-n-drop-as-text (event)
  "Drop the data in EVENT as text."
  (interactive "e")
  (gtk4-drag-n-drop event nil t))

(defun gtk4-drag-n-drop-as-text-other-frame (event)
  "Drop the data in EVENT as text in a new frame."
  (interactive "e")
  (gtk4-drag-n-drop event t t))

(global-set-key [drag-n-drop] 'gtk4-drag-n-drop)
(global-set-key [C-drag-n-drop] 'gtk4-drag-n-drop-other-frame)
(global-set-key [M-drag-n-drop] 'gtk4-drag-n-drop-as-text)
(global-set-key [C-M-drag-n-drop] 'gtk4-drag-n-drop-as-text-other-frame)

;;;; Frame-related functions.

;; gtk4term.c
(defvar gtk4-alternate-modifier)
(defvar gtk4-right-alternate-modifier)
(defvar gtk4-right-command-modifier)
(defvar gtk4-right-control-modifier)

;; You say tomAYto, I say tomAHto..
(defvaralias 'gtk4-option-modifier 'gtk4-alternate-modifier)
(defvaralias 'gtk4-right-option-modifier 'gtk4-right-alternate-modifier)

(defun gtk4-do-hide-emacs ()
  (interactive)
  (gtk4-hide-emacs t))

(declare-function gtk4-hide-others "gtk4fns.c" ())

(defun gtk4-do-hide-others ()
  (interactive)
  (gtk4-hide-others))

(declare-function gtk4-emacs-info-panel "gtk4fns.c" ())

(defun gtk4-do-emacs-info-panel ()
  (interactive)
  (gtk4-emacs-info-panel))

(defun gtk4-next-frame ()
  "Switch to next visible frame."
  (interactive)
  (other-frame 1))

(defun gtk4-prev-frame ()
  "Switch to previous visible frame."
  (interactive)
  (other-frame -1))

;; Frame will be focused anyway, so select it
;; (if this is not done, mode line is dimmed until first interaction)
;; FIXME: Sounds like we're working around a bug in the underlying code.
(add-hook 'after-make-frame-functions 'select-frame)

(defvar tool-bar-mode)
(declare-function tool-bar-mode "tool-bar" (&optional arg))

;; Based on a function by David Reitter <dreitter@inf.ed.ac.uk> ;
;; see https://lists.gnu.org/archive/html/emacs-devel/2005-09/msg00681.html .
(defun gtk4-toggle-toolbar (&optional frame)
  "Switches the tool bar on and off in frame FRAME.
 If FRAME is nil, the change applies to the selected frame."
  (interactive)
  (modify-frame-parameters
   frame (list (cons 'tool-bar-lines
		       (if (> (or (frame-parameter frame 'tool-bar-lines) 0) 0)
				   0 1)) ))
  (if (not tool-bar-mode) (tool-bar-mode t)))


;;;; Dialog-related functions.

;; Ask user for confirm before printing.  Due to Kevin Rodgers.
(defun gtk4-print-buffer ()
  "Interactive front-end to `print-buffer': asks for user confirmation first."
  (interactive)
  (if (and (called-interactively-p 'interactive)
           (or (listp last-nonmenu-event)
               (and (char-or-string-p (event-basic-type last-command-event))
                    (memq 'super (event-modifiers last-command-event)))))
      (let ((last-nonmenu-event (if (listp last-nonmenu-event)
                                    last-nonmenu-event
                                  ;; Fake it:
                                  `(mouse-1 POSITION 1))))
        (if (y-or-n-p (format "Print buffer %s? " (buffer-name)))
            (print-buffer)
	  (error "Canceled")))
    (print-buffer)))

;;;; Font support.

;; Needed for font listing functions under both backend and normal
(setq scalable-fonts-allowed t)

;; Set to use font panel instead
(declare-function gtk4-popup-font-panel "gtk4fns.c" (&optional frame))
(defalias 'x-select-font 'gtk4-popup-font-panel "Pop up the font panel.
This function has been overloaded in Nextstep.")
(defalias 'mouse-set-font 'gtk4-popup-font-panel "Pop up the font panel.
This function has been overloaded in Nextstep.")

;; gtk4term.c
(defvar gtk4-input-font)
(defvar gtk4-input-fontsize)

(defun gtk4-respond-to-change-font ()
  "Respond to changeFont: event, expecting `gtk4-input-font' and\n\
`gtk4-input-fontsize' of new font."
  (interactive)
  (modify-frame-parameters (selected-frame)
                           (list (cons 'fontsize gtk4-input-fontsize)))
  (modify-frame-parameters (selected-frame)
                           (list (cons 'font gtk4-input-font)))
  (set-frame-font gtk4-input-font))


;; Default fontset.  This is mainly here to show how a fontset
;; can be set up manually.  Ordinarily, fontsets are auto-created whenever
;; a font is chosen by
(defvar gtk4-standard-fontset-spec
  ;; Only some code supports this so far, so use uglier XLFD version
  ;; "-gtk4-*-*-*-*-*-10-*-*-*-*-*-fontset-standard,latin:Courier,han:Kai"
  (mapconcat 'identity
             '("-*-Monospace-*-*-*-*-10-*-*-*-*-*-fontset-standard"
               "latin:-*-Courier-*-*-*-*-10-*-*-*-*-*-iso10646-1"
               "han:-*-Kai-*-*-*-*-10-*-*-*-*-*-iso10646-1"
               "cyrillic:-*-Trebuchet$MS-*-*-*-*-10-*-*-*-*-*-iso10646-1")
             ",")
  "String of fontset spec of the standard fontset.
This defines a fontset consisting of the Courier and other fonts.
See the documentation of `create-fontset-from-fontset-spec' for the format.")


;;;; Pasteboard support.

(define-obsolete-function-alias 'gtk4-store-cut-buffer-internal
  'gui-set-selection "24.1")


(defun gtk4-copy-including-secondary ()
  (interactive)
  (call-interactively 'kill-ring-save)
  (gui-set-selection 'SECONDARY (buffer-substring (point) (mark t))))

(defun gtk4-paste-secondary ()
  (interactive)
  (insert (gui-get-selection 'SECONDARY)))


;;;; Color support.

;; Functions for color panel + drag
(defun gtk4-face-at-pos (pos)
  (let* ((frame (car pos))
         (frame-pos (cons (cadr pos) (cddr pos)))
         (window (window-at (car frame-pos) (cdr frame-pos) frame))
         (window-pos (coordinates-in-window-p frame-pos window))
         (buffer (window-buffer window))
         (edges (window-edges window)))
    (cond
     ((not window-pos)
      nil)
     ((eq window-pos 'mode-line)
      'mode-line)
     ((eq window-pos 'vertical-line)
      'default)
     ((consp window-pos)
      (with-current-buffer buffer
        (let ((p (car (compute-motion (window-start window)
                                      (cons (nth 0 edges) (nth 1 edges))
                                      (window-end window)
                                      frame-pos
                                      (- (window-width window) 1)
                                      nil
                                      window))))
          (cond
           ((eq p (window-point window))
            'cursor)
           ((and mark-active (< (region-beginning) p) (< p (region-end)))
            'region)
           (t
	    (let ((faces (get-char-property p 'face window)))
	      (if (consp faces) (car faces) faces)))))))
     (t
      nil))))

(defun gtk4-suspend-error ()
  ;; Don't allow suspending if any of the frames are GTK4 frames.
  (if (memq 'gtk4 (mapcar 'window-system (frame-list)))
      (error "Cannot suspend Emacs while a GTK4 GUI frame exists")))


;; Set some options to be as Nextstep-like as possible.
(setq frame-title-format t
      icon-title-format t)


(defvar gtk4-initialized nil
  "Non-nil if pure-GTK windowing has been initialized.")

(declare-function x-handle-args "common-win" (args))
(declare-function x-open-connection "gtk4fns.c"
                  (display &optional xrm-string must-succeed))
(declare-function gtk4-set-resource "gtk4fns.c" (owner name value))

;; Do the actual pure-GTK Windows setup here; the above code just
;; defines functions and variables that we use now.
(cl-defmethod window-system-initialization (&context (window-system gtk4)
                                            &optional display)
  "Initialize Emacs for pure-GTK windowing."
  (cl-assert (not gtk4-initialized))

  ;; PENDING: not needed?
  (setq command-line-args (x-handle-args command-line-args))

  ;; Make sure we have a valid resource name.
  (or (stringp x-resource-name)
      (let (i)
	(setq x-resource-name (invocation-name))

	;; Change any . or * characters in x-resource-name to hyphens,
	;; so as not to choke when we use it in X resource queries.
	(while (setq i (string-match "[.*]" x-resource-name))
	  (aset x-resource-name i ?-))))

  ;; Setup the default fontset.
  (create-default-fontset)
  ;; Create the standard fontset.
  (condition-case err
      (create-fontset-from-fontset-spec gtk4-standard-fontset-spec t)
    (error (display-warning
            'initialization
            (format "Creation of the standard fontset failed: %s" err)
            :error)))

  (x-open-connection (or display
                         x-display-name)
		     x-command-line-resources
		     ;; Exit Emacs with fatal error if this fails and we
		     ;; are the initial display.
                     (= (length (frame-list)) 0))

  ;; FIXME: This will surely lead to "MODIFIED OUTSIDE CUSTOM" warnings.
  ; (menu-bar-mode (if (get-lisp-resource nil "Menus") 1 -1))

  ;; Mac OS X Lion introduces PressAndHold, which is unsupported by this port.
  ;; See this thread for more details:
  ;; https://lists.gnu.org/archive/html/emacs-devel/2011-06/msg00505.html
  ; (gtk4-set-resource nil "ApplePressAndHoldEnabled" "NO")

  (x-apply-session-resources)

  ;; Don't let Emacs suspend under GTK4.
  (add-hook 'suspend-hook 'gtk4-suspend-error)

  (setq gtk4-initialized t))

;; Any display name is OK.
(add-to-list 'display-format-alist '(".*" . gtk4))
(cl-defmethod handle-args-function (args &context (window-system gtk4))
  (x-handle-args args))

(cl-defmethod frame-creation-function (params &context (window-system gtk4))
  (x-create-frame-with-faces params))

(declare-function gtk4-own-selection-internal "gtk4select.c" (selection value &optional frame))
(declare-function gtk4-disown-selection-internal "gtk4select.c" (selection &optional time_object terminal))
(declare-function gtk4-selection-owner-p "gtk4select.c" (&optional selection terminal))
(declare-function gtk4-selection-exists-p "gtk4select.c" (&optional selection terminal))
(declare-function gtk4-get-selection-internal "gtk4select.c" (selection-symbol target-type &optional time_stamp terminal))

(cl-defmethod gui-backend-set-selection (selection value
                                         &context (window-system gtk4))
  (if value (gtk4-own-selection-internal selection value)
    (gtk4-disown-selection-internal selection)))

(cl-defmethod gui-backend-selection-owner-p (selection
                                             &context (window-system gtk4))
  (gtk4-selection-owner-p selection))

(cl-defmethod gui-backend-selection-exists-p (selection
                                              &context (window-system gtk4))
  (gtk4-selection-exists-p selection))

(cl-defmethod gui-backend-get-selection (selection-symbol target-type
                                         &context (window-system gtk4))
  (gtk4-get-selection-internal selection-symbol target-type))

(provide 'gtk4-win)
(provide 'term/gtk4-win)

;;; gtk4y-win.el ends here
