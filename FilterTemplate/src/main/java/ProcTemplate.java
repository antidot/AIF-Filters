import net.antidot.paf.Configuration;
import net.antidot.paf.Document;
import net.antidot.paf.Handle;
import net.antidot.paf.Layer;
import net.antidot.paf.Pipe;
import net.antidot.paf.ProcessorFilter;
import net.antidot.protobuf.event.EventProtos;
import net.antidot.protobuf.paf.PaFProtos.Status;
import net.antidot.protobuf.paf.layer.LayerType.Type;



/**
 * @author lm
 *
 */
public class ProcTemplate extends ProcessorFilter {

    /**
     * @param configuration
     * @param handle
     */
    public ProcTemplate(Configuration configuration, Handle handle) {
	super(configuration, handle);
    }

    /* (non-Javadoc)
     * @see net.antidot.paf.ProcessorFilter#init()
     */
    @Override
    public void init() {
	System.out.println("Initialize me");
    }

    /* (non-Javadoc)
     * @see net.antidot.paf.ProcessorFilter#process(net.antidot.paf.Document)
     */
    @Override
    public void process(Document doc) {
	System.out.println("Doc: "+doc.get_doc_id());
	doc.set_status(Status.OK);
	Layer layer = doc.get_layer(Type.CONTENTS);
	System.out.println(layer.get_data_as_string());
	
	handle.log(EventProtos.Type.INFO,
		   "I need to talk with my Back Office",
		   EventProtos.Level.TUNING);	
    }
    
    public static void main(String[] args) {
	int return_code = Pipe.start_filter(ProcTemplate.class, args);
	System.exit(return_code);
    }
}
