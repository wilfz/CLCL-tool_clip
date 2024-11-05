/* -----------------------------------------------------------------------------
 *  Projekt: BusinessCentre
 *  Bereich: common
 * -----------------------------------------------------------------------------
 *  (c) copyright 2024 by Mesalvo Mannheim GmbH
 * -----------------------------------------------------------------------------
 * procedure:       replace_clipitem()
 * -----------------------------------------------------------------------------
 * function :	
 * -----------------------------------------------------------------------------
 * remarks:	
 * -----------------------------------------------------------------------------
 */

if exists (select 1 from sysobjects where name = 'replace_clipitem' and type = 'P')
begin
	drop procedure replace_clipitem
	print 'replace_clipitem deleted ...'
end
go
--

create procedure replace_clipitem ( 
		@title varchar(255), @parent varchar(255), @sort int null, @itemtype int null, 
		@itemformat int null, @formatname varchar(255) null, @textcontent varchar null, @modified datetime null
	)
as
begin
	-- $Revision:$
	/* sql-server be quiet */
	/* ------------------- */
	set nocount on
	declare @retcode int 
	set @retcode = 0
	
begin try
	--if not exists( select * from clipitem where title = @title and parent = @parent)
	insert into clipitem
		select @title, @parent, @sort, @itemtype, @itemformat, @formatname, @textcontent, @modified 
		where not exists( select * from clipitem where title = @title and parent = @parent);

	set @retcode = @@error
	if @retcode != 0
		goto abbruch

	update clipitem set title=@title, parent=@parent, sort=@sort, itemtype=3 
	where title = @title and parent = @parent;

	set @retcode = @@error
	if @retcode != 0
		goto abbruch
		
	return

abbruch:
	return @retcode
end try
begin catch
	raiserror ('Abbruch mit Fehler %d: %s', 16,1, @@error, 'replace_clipitem')
end catch

end
go

/* security anforderungen eintragen */
/* -------------------------------- */
exec tp_addobjectsec 'replace_clipitem', 0
go
exec tp_addobjectsec 'replace_clipitem', 1
go
exec tp_addobjectsec 'replace_clipitem', 2
go
exec tp_addobjectsec 'replace_clipitem', 3
go
exec tp_addobjectsec 'replace_clipitem', 4
go

/* debug help */
/* ---------- */
print 'replace_clipitem: generiert'
go
grant exec on replace_clipitem to public
go
grant exec on replace_clipitem to sbsadmin with grant option
go
/*
setuser 'lfb'
declare @retval int
set @retval = 0
exec replace_clipitem '01', 42, @retval = @retval output 
select @retval
setuser
*/
